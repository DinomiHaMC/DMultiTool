#include "FileManagerApp.h"
#include "../services/ShellParser.h"
void FileManagerApp::shell() {
  shellOpen=true;
  ui.rows("MtSh: "+path,shellOutput,false);
  auto& p=ui.model();
  p.items.insert(p.items.begin(),{"Command","pwd: "+path,Icon::Script,[this]{ui.textInput("MtSh command","",[this](String text){command(text);},768);}});
  p.selected=0;p.hint="OK Command | Hold OK Files";
  p.onBack=[this]{shellOpen=false;browse(path,offset);};
  ui.dirty=true;
}
void FileManagerApp::command(const String& source) {
  std::vector<std::string> args;
  if(!MtSh::tokenize(source.c_str(),args)||args.empty()) { shellOutput="Parse error: use quotes for spaces";shell();return; }
  String cmd=args[0].c_str();shellOutput="Invalid command or arguments: help";
  auto resolved=[this,&args](size_t index,String& out) {
    std::string value;
    if(index>=args.size()||!MtSh::path(path.c_str(),args[index],value))return false;
    out=value.c_str();return true;
  };
  String a,b;
  if(cmd=="exit") { browse(path,offset);return; }
  if(cmd=="help")shellOutput="pwd | ls [path] [offset]\ncd path | open path\ncat file [offset] | info path\nmkdir path | touch file\nwrite file \"text\"\nappend file \"text\"\ncp source target (<=1MB)\nmv / rename source target\nrm file | rmdir empty-dir\nwget / download URL file\ncancel | exit\nQuotes and relative paths supported";
  else if(cmd=="pwd"&&args.size()==1)shellOutput=path;
  else if(cmd=="cancel"&&args.size()==1) { s.download.cancel();shellOutput="Cancellation requested"; }
  else if(cmd=="ls"&&args.size()<=3) {
    a=path;uint32_t page=0;bool valid=args.size()<2||resolved(1,a);
    if(args.size()==3) { char* end=nullptr;unsigned long n=strtoul(args[2].c_str(),&end,10);valid=valid&&end&&!*end&&args[2][0]!='-';page=n; }
    if(valid&&s.sd.list(a,page)) {
      shellOutput=a+"\n";
      size_t shown=0;
      for(;shown<s.sd.count&&shellOutput.length()<1800;shown++) {
        const auto& e=s.sd.entries[shown];shellOutput+=String(e.directory?"[dir] ":"      ")+e.name+" "+String((unsigned long)e.size)+"\n";
      }
      if(shown<s.sd.count||s.sd.truncated)shellOutput+="More: ls \""+a+"\" "+String(page+shown);
    } else shellOutput="Directory read failed";
  }
  else if((cmd=="cd"||cmd=="open")&&args.size()==2&&resolved(1,a)) {
    File f=SD.open(a);bool dir=f&&f.isDirectory();bool exists=bool(f);f.close();
    if(!exists)shellOutput="Path not found";
    else if(cmd=="open") { if(dir)browse(a);else context(a,false);return; }
    else if(dir) { path=a;offset=0;shellOutput="cwd: "+path; }
    else shellOutput="Not a directory";
  }
  else if(cmd=="cat"&&(args.size()==2||args.size()==3)&&resolved(1,a)) {
    uint32_t position=0,next=0;bool valid=true;
    if(args.size()==3) { char* end=nullptr;position=strtoul(args[2].c_str(),&end,10);valid=end&&!*end&&args[2][0]!='-'; }
    if(!valid||!s.sd.readPage(a,position,shellOutput,next))shellOutput="Read failed";
    else if(next)shellOutput+="\nNext: cat \""+a+"\" "+String(next);
  }
  else if(cmd=="info"&&args.size()==2&&resolved(1,a)) {
    File f=SD.open(a);
    shellOutput=f?a+"\n"+(f.isDirectory()?"Directory":"File")+"\nSize: "+String((unsigned long)f.size()):"Path not found";f.close();
  }
  else if(cmd=="mkdir"&&args.size()==2&&resolved(1,a))shellOutput=!SD.exists(a)&&SD.mkdir(a)?"Directory created":"mkdir failed / already exists";
  else if(cmd=="touch"&&args.size()==2&&resolved(1,a))shellOutput=s.sd.writeNew(a,"")?"File created":"touch failed / already exists";
  else if((cmd=="write"||cmd=="append")&&args.size()==3&&resolved(1,a)) {
    File f=SD.open(a,cmd=="append"?FILE_APPEND:FILE_WRITE);
    shellOutput=f&&f.print(args[2].c_str())==args[2].size()?"Written":"Write failed";f.close();
  }
  else if((cmd=="mv"||cmd=="rename")&&args.size()==3&&resolved(1,a)&&resolved(2,b)&&a!="/")shellOutput=!SD.exists(b)&&SD.rename(a,b)?"Renamed":"Rename failed / target exists";
  else if((cmd=="rm"||cmd=="rmdir")&&args.size()==2&&resolved(1,a)&&a!="/") {
    shellOutput=cmd=="rm"?(s.sd.removeFile(a)?"Deleted":"Delete failed"):(SD.rmdir(a)?"Directory removed":"rmdir failed: directory must be empty");
  }
  else if(cmd=="cp"&&args.size()==3&&resolved(1,a)&&resolved(2,b)) {
    File in=SD.open(a),out;bool created=false;
    bool ok=in&&!in.isDirectory()&&in.size()<=1024*1024&&!SD.exists(b);
    if(ok) { out=SD.open(b,FILE_WRITE);ok=bool(out);created=ok; }
    uint8_t bytes[512];
    while(ok&&in.available()) { int n=in.read(bytes,sizeof(bytes));ok=n>0&&out.write(bytes,n)==size_t(n);yield(); }
    in.close();out.close();if(!ok&&created)SD.remove(b);
    shellOutput=ok?"Copied":"Copy failed / exists / >1MB";
  }
  else if((cmd=="wget"||cmd=="download")&&args.size()==3&&resolved(2,a)) {
    downloadRevision=s.download.revision;
    bool ok=s.download.start(args[1].c_str(),a);
    shellOutput=ok?"Downloading. Type cancel to stop":s.download.message;
  }
  shell();
}
void FileManagerApp::update() {
  if(shellOpen&&s.download.revision!=downloadRevision) { downloadRevision=s.download.revision;shellOutput=s.download.message;if(!ui.isInput())shell(); }
}
void FileManagerApp::home()  {
  browse("/");
}
void FileManagerApp::browse(const String& directory,uint32_t start)  {
  path=directory;
  shellOpen=false;
  offset=start;
  if(!s.sd.list(path,start))  {
    ui.message("Storage","SD card not mounted / directory error");
    return;
  }
  ui.reset();
  auto p=menu(path);
  p.hint="Arrows Move | OK Open | Hold OK Back";
  p.onBack=[this]  {
    if(path=="/")ctx.apps->launcher();
    else  {
      int slash=path.lastIndexOf('/');
      browse(slash<=0?"/":path.substring(0,slash));
    }
  };
  if(offset)item(p,"Previous entries",[this]  {
    browse(path,offset-Config::MaxEntries);
  }
  );
  for(size_t i=0;i<s.sd.count;i++)  {
    auto e=s.sd.entries[i];
    bool directory=e.directory;
    String full=path=="/"?"/"+String(e.name):path+"/"+e.name;
    String detail=e.directory?"Directory":String((unsigned long)e.size)+" bytes";
    p.items.push_back(  {
      e.name,detail,directory?Icon::Folder:Icon::File,[this,full,directory]  {
        if(directory)browse(full);else context(full,false);
      },[this,full,directory]  {
        context(full,directory);
      }
    }
    );
  }
  if(s.sd.truncated)item(p,"Next entries",[this]  {
    browse(path,offset+Config::MaxEntries);
  }
  );
  item(p,"Open MtSh",[this]  {
    shell();
  }
  );
  if(path!="/")item(p,"Folder actions",[this]{context(path,true);});
  ui.page(std::move(p),false);
}
void FileManagerApp::mkdir()  {
  ui.textInput("Directory name","",[this](String name)  {
    if(name.isEmpty()||name.indexOf('/')>=0||name=="."||name=="..")  {
      report(false,"","Invalid name");return;
    }
    String full=path=="/"?"/"+name:path+"/"+name;report(s.sd.mounted&&!SD.exists(full)&&SD.mkdir(full),"Directory created");browse(path,offset);
  },64);
}
void FileManagerApp::context(const String& full,bool directory)  {
  auto p=menu("File actions");
  item(p,"Open",[this,full,directory]  {
    if(directory)browse(full);else openFile(full);
  }
  );
  item(p,"Info",[this,full]  {
    File f=SD.open(full);if(!f)  {
      report(false);return;
    }
    String info=full+"\nSize: "+String((unsigned long)f.size())+" bytes\n"+(f.isDirectory()?"Directory":"File");f.close();ui.message("Info",info);
  }
  );
  item(p,"Rename",[this,full]  {
    ui.textInput("New name",full.substring(full.lastIndexOf('/')+1),[this,full](String name)  {
      if(name.isEmpty()||name.indexOf('/')>=0||name=="."||name=="..")  {
        report(false,"","Invalid name");return;
      }
      String parent=full.substring(0,full.lastIndexOf('/'));String target=parent+"/"+name;report(!SD.exists(target)&&SD.rename(full,target),"Renamed");browse(parent.isEmpty()?"/":parent);
    },64);
  }
  );
  if(!directory)item(p,"Delete",[this,full]  {
    ui.confirm("Delete file?",full.substring(full.lastIndexOf('/')+1),[this,full]  {
      bool ok=s.sd.removeFile(full);browse(path,offset);report(ok,"Deleted");
    }
    );
  }
  );
  item(p,"Open MtSh",[this]  {
    shell();
  }
  );
  ui.page(std::move(p));
}
void FileManagerApp::openFile(const String& file)  {
  String lower=file;
  lower.toLowerCase();
  bool supported=false;
  for(const char* ext:  {
    ".txt",".log",".json",".csv",".ir",".nfc",".script",".py",".md"
  }
  )if(lower.endsWith(ext))supported=true;
  if(!supported)  {
    ui.message("Viewer","Unsupported binary file");
    return;
  }
  opened=file;
  pageOffset=0;
  positions.clear();
  view();
}
void FileManagerApp::view()  {
  String text;
  if(!s.sd.readPage(opened,pageOffset,text,nextOffset))  {
    report(false,"","Read failed");
    return;
  }
  ui.rows(opened.substring(opened.lastIndexOf('/')+1),text,false);
  auto& p=ui.model();
  p.selected=0;
  p.hint="RIGHT next | Hold OK previous";
  p.onBack=[this]  {
    if(positions.empty())browse(path,offset);
    else  {
      pageOffset=positions.back();
      positions.pop_back();
      view();
    }
  };
  for(auto& row:p.items)row.context=[this]  {
    if(nextOffset)  {
      if(positions.size()==128)positions.erase(positions.begin());
      positions.push_back(pageOffset);
      pageOffset=nextOffset;
      view();
    }
  };
  if(nextOffset)item(p,"Next page",[this]  {
    if(positions.size()==128)positions.erase(positions.begin());
    positions.push_back(pageOffset);pageOffset=nextOffset;view();
  }
  );
  ui.dirty=true;
}
