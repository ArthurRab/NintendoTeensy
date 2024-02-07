/*
Copyright (c) 2014-2021 NicoHood
See the readme for credit to other people.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "Gamecube_N64.h"

//================================================================================
// Gamecube/N64 I/O functions
//================================================================================

uint8_t gc_n64_send_get(const uint8_t pin, uint8_t* command, const uint8_t commandLen,
    uint8_t* report, const uint8_t reportLen){
    // send the command
    gc_n64_send(command, commandLen, pin);
    // read in data
    uint8_t receivedBytes = gc_n64_get(report, reportLen, pin, 100);

    // end of time sensitive code

    // return received length
    return receivedBytes;
}

#define debug false

bool wait_for_edge(uint8_t pin, uint8_t dir, int timeout){
  auto start = micros();
  auto limit = start + timeout;
  while(digitalRead(pin) == dir && micros() <= limit);
  return (micros() <= limit);
}

bool wait_for_down_edge(uint8_t pin, int timeout){
  if (!wait_for_edge(pin, LOW, timeout)){
    return false;
  }
  return wait_for_edge(pin, HIGH, timeout);
}

bool wait_for_up_edge(uint8_t pin, int timeout){
  if (!wait_for_edge(pin, HIGH, timeout)){
    return false;
  }
  return wait_for_edge(pin, LOW, timeout);
}

uint8_t read_bit(uint8_t pin, int timeout){
  auto worked = wait_for_down_edge(pin,timeout);
  int start = micros();
  worked = worked && wait_for_up_edge(pin,timeout);
  int end = micros();

  if (!worked){
    return 2;
  }

  return (end - start <= 2);
}

void write_bit(uint8_t pin, bool bit){
  digitalWrite(pin, LOW);
  if(bit){
    delayMicroseconds(1);
  }else{
    delayNanoseconds(2750);
  }
  digitalWrite(pin, HIGH);
  if(bit){
    delayNanoseconds(2750);
  }else{
    delayMicroseconds(1);
  }
}


void write_stop_bit(uint8_t pin){
  digitalWrite(pin, LOW);
  delayMicroseconds(1);
  digitalWrite(pin, HIGH);
  //delayNanoseconds(500);
}

void expect_bit(uint8_t pin, bool val, int timeout){
  if(debug){
    auto bit = read_bit(pin, timeout);
    if (bit == 2){
      Serial.println("TIMEOUT");
    }else if (bit != val){
      Serial.printf("Got unexpected bit value %d on pin %d\n", !val, pin);
    }
  }
}

void expect_stop_bit(uint8_t pin, uint8_t timeout){
  expect_bit(pin, 1, timeout);
}

void print_buffer(const uint8_t* buff, uint8_t len){
   for(int i=0;i<len;i++){
    for(int j = 0; j<8;j++){
      // TODO: MAYBE SWAP ORDER WITHIN BYTE
      auto k = (7-j);
      bool bit = (buff[i] >> k)%2;
      Serial.println(bit);
    }
  }
}



void gc_n64_send(const uint8_t* buff, uint8_t len, const uint8_t pin)
{

  pinMode(pin, OUTPUT);
  for(int i=0;i<len;i++){
    for(int j = 0; j<8;j++){
      // TODO: MAYBE SWAP ORDER WITHIN BYTE
      auto k = (7-j);
      bool bit = (buff[i] >> k)%2;
      write_bit(pin, bit);

    }
  }
  write_stop_bit(pin);
  pinMode(pin, INPUT_PULLUP);

}

/**
* Read bytes from the gamecube controller
* listen for the expected bytes of data back from the controller and
* and pack it into the buff
*/
uint8_t gc_n64_get(uint8_t* buff, uint8_t len, const uint8_t pin, int initial_timeout)
{

  int timeout = 7;

  pinMode(pin, INPUT_PULLUP);
  uint8_t receivedBytes = 0;

  uint8_t bit = 0;
  int j = 0;
  for(int i=0;i<len;i++){
    buff[i]=0;
    for(j = 0; j<8;j++){
      auto to = timeout;
      if(j == 0 && i == 0){
        to = initial_timeout;
      }
      bit = read_bit(pin, to);
      if (bit == 2){
        break;
      }
      else{
        buff[i] += bit << (7-j);
      }
     }

    if(bit == 2){
      break;
    }else{
      receivedBytes++;
    }
  }



  return receivedBytes;
}
