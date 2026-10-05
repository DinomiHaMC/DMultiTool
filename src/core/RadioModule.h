#pragma once
class RadioModule  {
  public:virtual ~RadioModule()=default;
  virtual bool begin()=0;
  virtual bool available()const=0;
};
class CC1101Module:public RadioModule  {
  public:bool begin()override  {
    return false;
  }
  bool available()const override  {
    return false;
  }
};
class NRF24Module:public RadioModule  {
  public:bool begin()override  {
    return false;
  }
  bool available()const override  {
    return false;
  }
};
