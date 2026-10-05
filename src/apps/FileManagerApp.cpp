#include "FileManagerApp.h"
void FileManagerApp::home()  {
  browse("/");
}
void FileManagerApp::browse(const String& directory,uint32_t start)  {
  path=directory;
  offset=start;
  if(!s.sd.list(path,start))  {
    ui.message("Storage","SD card not mounted / directory error");
    return;
  }
  ui.reset();
  auto p=menu(path);
  p.hint="OK Open  RIGHT Actions";
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
    String full=path=="/"?"/"+String(e.name):path+"/"+e.name;
    String detail=e.directory?"Directory":String((unsigned long)e.size)+" bytes";
    p.items.push_back(  {
      e.name,detail,e.directory?Icon::Folder:Icon::File,[this,full,e]  {
        if(e.directory)browse(full);else openFile(full);
      },[this,full,e]  {
        context(full,e.directory);
      }
    }
    );
  }
  if(s.sd.truncated)item(p,"Next entries",[this]  {
    browse(path,offset+Config::MaxEntries);
  }
  );
  item(p,"Create directory",[this]  {
    mkdir();
  }
  );
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
      String target=path=="/"?"/"+name:path+"/"+name;report(!SD.exists(target)&&SD.rename(full,target),"Renamed");browse(path,offset);
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
  item(p,"Create directory",[this]  {
    mkdir();
  }
  );
  ui.page(std::move(p));
}
void FileManagerApp::openFile(const String& file)  {
  String lower=file;
  lower.toLowerCase();
  bool supported=false;
  for(const char* ext:  {
    ".txt",".log",".json",".csv",".ir",".nfc",".script"
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
  p.hint="OK/RIGHT page   LEFT previous";
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
