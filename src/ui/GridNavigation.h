#pragma once
#include <algorithm>
namespace GridNavigation {
inline int move(int selected,int count,int columns,int dx,int dy,bool wrap) {
  if(count<=0)return 0;
  int col=selected%columns,row=selected/columns;
  if(dy) {
    int next=selected+dy*columns;
    if(next>=0&&next<count)return next;
    if(!wrap)return selected;
    return dy>0?col:col+(count-1-col)/columns*columns;
  }
  int first=row*columns,last=std::min(first+columns-1,count-1);
  int next=selected+dx;
  if(next<first)return wrap?last:selected;
  if(next>last)return wrap?first:selected;
  return next;
}
}
