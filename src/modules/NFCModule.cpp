#include "NFCModule.h"
#include "PN532Polling.h"
#include "../pins.h"
#include "../config.h"
#include "../services/NDEFCodec.h"
#include "../services/ClassicNDEF.h"
namespace {
class ClassicCard:public ClassicNDEF::Card {
  Adafruit_PN532& reader;
  const std::atomic<uint32_t>& token;
  uint32_t expected;
  uint8_t uid[4]{};
public:
  ClassicCard(Adafruit_PN532& r,const TagInfo& tag,const std::atomic<uint32_t>& t,uint32_t e):reader(r),token(t),expected(e) {
    // PN532 MIFARE authentication takes the last 4 bytes, including 7-byte UIDs.
    memcpy(uid,tag.uid+tag.length-4,4);
  }
  bool authenticate(uint8_t sector,const uint8_t key[6])override {
    if(cancelled())return false;
    uint8_t copy[6];memcpy(copy,key,6);
    return reader.mifareclassic_AuthenticateBlock(uid,4,sector*4,0,copy);
  }
  bool read(uint8_t block,uint8_t data[16])override { return !cancelled()&&reader.mifareclassic_ReadDataBlock(block,data); }
  bool write(uint8_t block,const uint8_t data[16])override {
    if(cancelled())return false;
    uint8_t copy[16];memcpy(copy,data,16);
    return reader.mifareclassic_WriteDataBlock(block,copy);
  }
  bool cancelled()const override { return token.load()!=expected; }
  void yield()override { vTaskDelay(1); }
};
}
void NFCModule::begin()  {
  Wire.begin(Pins::SDA,Pins::SCL);
  Wire.setTimeOut(25);
  bus=xSemaphoreCreateMutex();
  tags=xQueueCreate(1,sizeof(TagInfo));
  busResults=xQueueCreate(1,sizeof(I2CResult));
  ndefJobs=xQueueCreate(1,sizeof(NDEFJob));
  ndefResults=xQueueCreate(1,sizeof(NDEFResult));
  if(!bus||!tags||!busResults||!ndefJobs||!ndefResults)  {
    LOG_ERROR("NFC","Allocation failed");
    return;
  }
  // begin wakes the PN532; an address probe beforehand can miss a sleeping chip.
  const bool started=reader.begin();
  const uint32_t firmware=started?reader.getFirmwareVersion():0;
  const bool sam=firmware&&reader.SAMConfig();
  const bool configured=sam&&PN532Polling::configure(reader,Wire,Pins::PN532_ADDRESS);
  available=started&&firmware&&sam&&configured;
  LOG_INFO("NFC","Init begin=%d firmware=%08lX SAM=%d RF=%d",started,(unsigned long)firmware,sam,configured);
  if(xTaskCreate(worker,"nfc",8192,this,1,&task)!=pdPASS)available=false;
  LOG_INFO("BOOT","PN532 %s",available?"found at 0x24":"not available");
}
void NFCModule::scan(bool enabled)  {
  if(!task)return;
  scanRequested=enabled;
  if(!enabled)  {
    token++;
    ndefBusy=false;
  }
  if(enabled) {
    LOG_INFO("NFC","Scan started: ISO14443A, I2C 0x24, timeout %u ms",PN532Polling::ResponseTimeoutMs);
    xTaskNotify(task,1,eSetValueWithOverwrite);
  }
  else xTaskNotify(task,2,eSetValueWithOverwrite);
}
void NFCModule::worker(void* arg)  {
  auto& self=*static_cast<NFCModule*>(arg);
  uint32_t bits;
  uint32_t searchStart=0;
  bool ndefPending=false;
  NDEFJob ndef;
  for(;;)  {
    if(xTaskNotifyWait(0,UINT32_MAX,&bits,pdMS_TO_TICKS(50))==pdTRUE)  {
      if(bits==1)  {
        self.active=true;
        ndefPending=false;
      }
      if(bits==2)  {
        self.active=false;
        if(ndefPending)  {
          NDEFResult result;
          result.token=ndef.token;
          strcpy(result.text,"Cancelled");
          xQueueOverwrite(self.ndefResults,&result);
          ndefPending=false;
        }
      }
      if(bits==5&&xQueueReceive(self.ndefJobs,&ndef,0)==pdTRUE)  {
        self.active=true;
        ndefPending=true;
        searchStart=millis();
      }
      if(bits==4)  {
        self.active=false;
        I2CResult result;
        if(xSemaphoreTake(self.bus,pdMS_TO_TICKS(100))==pdTRUE)  {
          for(uint8_t a=1;a<127;a++)  {
            Wire.beginTransmission(a);
            if(Wire.endTransmission()==0)result.addresses[result.count++]=a;
            vTaskDelay(1);
          }
          xSemaphoreGive(self.bus);
        }
        xQueueOverwrite(self.busResults,&result);
      }
    }
    if(ndefPending&&millis()-searchStart>15000)  {
      NDEFResult result;
      result.token=ndef.token;
      strcpy(result.text,"Tag timeout (15s)");
      xQueueOverwrite(self.ndefResults,&result);
      ndefPending=false;
      self.active=false;
    }
    if(!self.active)continue;
    TagInfo tag;
    if(xSemaphoreTake(self.bus,pdMS_TO_TICKS(30))==pdTRUE)  {
      bool ok=PN532Polling::selectTag(self.reader,Wire,Pins::PN532_ADDRESS,PN532_MIFARE_ISO14443A,tag.uid,tag.length,tag.atqa,tag.sak);
      if(ok&&ndefPending)  {
        NDEFResult result;
        result.token=ndef.token;
        self.ndefOperation(ndef,result,tag);
        xQueueOverwrite(self.ndefResults,&result);
        ndefPending=false;
      }
      xSemaphoreGive(self.bus);
      if(ok&&tag.length>0&&tag.length<=7)  {
        size_t n=0;
        for(uint8_t i=0;i<tag.length;i++)n+=snprintf(tag.hex+n,sizeof(tag.hex)-n,i?":%02X":"%02X",tag.uid[i]);
        LOG_INFO("NFC","Target UID %s SAK=%02X ATQA=%04X",tag.hex,tag.sak,tag.atqa);
        xQueueOverwrite(self.tags,&tag);
        self.active=false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
bool NFCModule::update()  {
  if(tags&&xQueueReceive(tags,&last,0)==pdTRUE)  {
    scanRequested=false;
    LOG_INFO("NFC","Tag detected %s",last.hex);
    return true;
  }
  return false;
}
bool NFCModule::requestBusScan()  {
  if(!task||busScanning)return false;
  busScanning=true;
  xTaskNotify(task,4,eSetValueWithOverwrite);
  return true;
}
bool NFCModule::updateBusScan(I2CResult& result)  {
  if(busResults&&xQueueReceive(busResults,&result,0)==pdTRUE)  {
    busScanning=false;
    return true;
  }
  return false;
}
bool NFCModule::readNdef()  {
  if(!available||ndefBusy||busScanning)return false;
  NDEFJob job;
  job.token=++token;
  ndefBusy=true;
  scanRequested=true;
  xQueueOverwrite(ndefJobs,&job);
  xTaskNotify(task,5,eSetValueWithOverwrite);
  return true;
}
bool NFCModule::writeNdef(const String& text,bool uri)  {
  if(!available||ndefBusy||busScanning||text.isEmpty()||text.length()>100)return false;
  NDEFJob job;
  job.token=++token;
  job.write=true;
  job.uri=uri;
  text.toCharArray(job.text,sizeof(job.text));
  ndefBusy=true;
  scanRequested=true;
  xQueueOverwrite(ndefJobs,&job);
  xTaskNotify(task,5,eSetValueWithOverwrite);
  return true;
}
bool NFCModule::updateNdef()  {
  NDEFResult result;
  if(ndefResults&&xQueueReceive(ndefResults,&result,0)==pdTRUE)  {
    if(result.token!=token)return false;
    ndefBusy=false;
    scanRequested=false;
    ndefOk=result.ok;
    ndefText=result.text;
    ndefRevision++;
    return true;
  }
  return false;
}
void NFCModule::ndefOperation(const NDEFJob& job,NDEFResult& result,const TagInfo& tag)  {
  if(job.token!=token)  {
    strcpy(result.text,"Cancelled");
    return;
  }
  // Standard Classic 1K is SAK 08 / ATQA 0004. Some compatible Classic
  // chips return SAK 00 despite the Classic ATQA; the card still must pass
  // MAD/key/access checks before any data block is changed.
  const bool classic=(tag.sak&0x08)!=0||(tag.sak==0&&tag.atqa==0x0004);
  if(classic) {
    ClassicCard card(reader,tag,token,job.token);
    auto outcome=ClassicNDEF::operate(card,job.write,job.text,job.uri);
    result.ok=outcome.ok;
    snprintf(result.text,sizeof(result.text),"%s",outcome.text.c_str());
    return;
  }
  if(tag.sak!=0) {
    snprintf(result.text,sizeof(result.text),"Unsupported NDEF tag SAK %02X; use NTAG or NDEF Classic 1K",tag.sak);
    return;
  }
  uint8_t cc[4];
  if(!reader.ntag2xx_ReadPage(3,cc)||cc[0]!=0xE1||(cc[1]>>4)!=1||(cc[3]>>4)!=0)  {
    strcpy(result.text,"Type 2 NDEF readable CC not found");
    return;
  }
  size_t capacity=(size_t)cc[2]*8;
  if(job.write)  {
    uint8_t version[8]=  {
    };
    if(!getVersion(version)||version[1]!=4||version[2]!=4||(version[6]!=0x0F&&version[6]!=0x11&&version[6]!=0x13))  {
      strcpy(result.text,"Writer supports verified NTAG213/215/216 only");
      return;
    }
    uint8_t lockPage=version[6]==0x0F?40:version[6]==0x11?130:226,locks[4],stat[4],cfg[4];
    if(cc[3]!=0||!reader.ntag2xx_ReadPage(2,stat)||stat[2]||stat[3]||!reader.ntag2xx_ReadPage(lockPage,locks)||locks[0]||locks[1]||locks[2]||!reader.ntag2xx_ReadPage(lockPage+1,cfg)||cfg[3]!=0xFF)  {
      strcpy(result.text,"Tag locked/protected; write refused");
      return;
    }
    auto bytes=NDEF::encode(job.text,job.uri);
    if(bytes.empty()||bytes.size()>capacity||bytes.size()>128)  {
      strcpy(result.text,"NDEF too large");
      return;
    }
    // Publish the TLV length last: interrupted writes leave an empty NDEF, not a valid partial record.
    if(job.token!=token)  {
      strcpy(result.text,"Cancelled");
      return;
    }
    uint8_t empty[4]=  {
      3,0,0xFE,0
    };
    if(!reader.ntag2xx_WritePage(4,empty))  {
      strcpy(result.text,"Write failed");
      return;
    }
    for(size_t i=4;i<bytes.size();i+=4)  {
      if(job.token!=token)  {
        strcpy(result.text,"Cancelled; NDEF empty");
        return;
      }
      if(!reader.ntag2xx_WritePage(4+i/4,bytes.data()+i))  {
        strcpy(result.text,"Partial write; tag NDEF is empty");
        return;
      }
      vTaskDelay(1);
    }
    if(job.token!=token)  {
      strcpy(result.text,"Cancelled; NDEF empty");
      return;
    }
    if(!reader.ntag2xx_WritePage(4,bytes.data()))  {
      strcpy(result.text,"Final write failed");
      return;
    }
    for(size_t i=0;i<bytes.size();i+=4)  {
      uint8_t verify[4];
      if(!reader.ntag2xx_ReadPage(4+i/4,verify)||memcmp(verify,bytes.data()+i,4))  {
        strcpy(result.text,"Read-back verification failed");
        return;
      }
    }
    result.ok=true;
    strcpy(result.text,"NDEF written and verified");
  }
  else  {
    uint8_t data[512]=  {
    };
    size_t limit=min(capacity,sizeof(data));
    for(size_t i=0;i<limit;i+=4)  {
      if(job.token!=token)  {
        strcpy(result.text,"Cancelled");
        return;
      }
      if(!reader.ntag2xx_ReadPage(4+i/4,data+i))  {
        strcpy(result.text,"Tag read failed");
        return;
      }
      vTaskDelay(1);
    }
    std::string decoded;
    if(!NDEF::decode(data,limit,decoded))  {
      strcpy(result.text,"No supported NDEF / exceeds 512 bytes");
      return;
    }
    result.ok=true;
    strncpy(result.text,decoded.c_str(),sizeof(result.text)-1);
  }
}
bool NFCModule::getVersion(uint8_t* version)  {
  // Adafruit inDataExchange uses a target number only initialized by inListPassiveTarget.
  // Use its public command/ACK transport with fixed target 1, then read the bounded I2C frame.
  uint8_t command[]=  {
    0x40,1,0x60
  };
  if(!reader.sendCommandCheckAck(command,3,100))return false;
  uint8_t response[20]=  {
  };
  size_t got=Wire.requestFrom((uint8_t)Pins::PN532_ADDRESS,(size_t)20);
  for(size_t i=0;i<got&&i<20;i++)response[i]=Wire.read();
  const uint8_t* frame=response+1;
  if(got!=20||response[0]!=1||frame[0]!=0||frame[1]!=0||frame[2]!=0xFF||frame[3]!=11||(uint8_t)(frame[3]+frame[4])!=0||frame[5]!=0xD5||frame[6]!=0x41||frame[7]!=0)return false;
  uint8_t sum=0;
  for(int i=5;i<=16;i++)sum+=frame[i];
  if(sum)return false;
  memcpy(version,frame+8,8);
  return true;
}
