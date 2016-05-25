/*  SampleProcessor

*/
#include <Arduino.h>
#include "SampleProcessor.h"

SampleProcessor::SampleProcessor()
  : _beg_index(0)
  , _end_index(0)
  , _size(0)
  , _capacity(0)
  , _dataBufferSize(DATA_BUFFER_SIZE)
{
}

SampleProcessor::STATUS SampleProcessor::begin(size_t capacity)
{
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::begin"));
  #endif
  //preallocate memory for all the slots
  _slots = (Packet*) calloc(capacity, sizeof(Packet));
  if (_slots == NULL){
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println("### Error failed to allocate memory for the queue!");
    #endif
    return ERROR_MEMALLOC_FAIL;
  } 
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.print("# \tallocated &_slots=");DEBUG_PORT.println((int) &_slots, HEX);
  #endif
  Packet *pkt_slot;
  for(size_t i=0; i < _capacity; i++){
    pkt_slot = &(_slots[i]); //pull out the slot by address
    //pkt_slot->data = (byte*) calloc(_dataBufferSize, sizeof(byte));
    pkt_slot->length = 0;
    pkt_slot->timestamp = 0;
    pkt_slot->flags  = 0x00;
  }
  _capacity = capacity;  //make sure to cache
  return SUCCESS;
}

SampleProcessor::STATUS SampleProcessor::end()
{
  //Packet *pkt_slot;
  //for(size_t i=0; i < _capacity; i++){
  //  pkt_slot = &(_slots[i]); //pull out the slot by address
  //  //free(pkt_slot->data);
  //}
  free(_slots);
  return SUCCESS;
}

SampleProcessor::STATUS SampleProcessor::reset()
{
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::reset"));
  #endif
  _size= 0;
  _beg_index = 0;
  _end_index = 0;
  return SUCCESS;
}

SampleProcessor::STATUS SampleProcessor::enqueue_rawdata(Packet& pkt)
{
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::enqueue"));
  #endif
  if ((_size + 1) <= _capacity){
    _put_at(_end_index, pkt);
     //adjust the size and indices
    _size++;
    _end_index = (_end_index + 1) % _capacity; //wrap around if needed
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println(F("# (enqueue) after copy"));
    DEBUG_PORT.print(F("# \t_end_index="));DEBUG_PORT.println(_end_index);
    DEBUG_PORT.print(F("# \t_size="));DEBUG_PORT.println(_size);
    #endif
    return SUCCESS;
  }
  else{
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println(F("\t### Error: Queue Overflow"));
    #endif
    return ERROR_QUEUE_OVERFLOW;
  }
}

SampleProcessor::STATUS SampleProcessor::dequeue_rawdata(Packet& pkt)
{
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::dequeue_rawdata"));
  #endif
  if (_size > 0){
    _get_from(_beg_index, pkt);
    //adjust the size and indices
    _beg_index = (_beg_index + 1) % _capacity; //wrap around if needed
    _size--;
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println(F("# (dequeue) after copy"));
    DEBUG_PORT.print(F("# \t_beg_index="));DEBUG_PORT.println(_beg_index);
    DEBUG_PORT.print(F("# \t_size="));DEBUG_PORT.println(_size);
    #endif
    return SUCCESS;
  }
  else{
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println(F("### Error: Queue Underflow"));
    #endif
    pkt.length = 0; //set to safe value
    return ERROR_QUEUE_UNDERFLOW;
  }
}

SampleProcessor::STATUS SampleProcessor::requeue_rawdata(Packet& pkt)
{
  //pushes packet onto the front of the queue
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::requeue_rawdata"));
  #endif
  if ((_size + 1) <= _capacity){
    //update the size and indices ahead of time
    _size++;
    _beg_index = (_beg_index == 0)? (_capacity - 1) : (_beg_index - 1); //wrap around if needed
    _put_at(_beg_index, pkt);
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println(F("# (requeue) after copy"));
    DEBUG_PORT.print(F("# \t_beg_index="));DEBUG_PORT.println(_beg_index);
    DEBUG_PORT.print(F("# \t_size="));DEBUG_PORT.println(_size);
    #endif
    return SUCCESS;
  }
  else{
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println(F("### Error: Queue Overflow"));
    #endif
    return ERROR_QUEUE_OVERFLOW;
  }
}

SampleProcessor::STATUS SampleProcessor::process(Packet& pkt)
{
  //pulls packet off queue and processes, overload in child class for unique behaviors
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::process"));
  #endif
  Packet raw_pkt;
  STATUS status = dequeue_rawdata(raw_pkt);
  if (status == SUCCESS){
    //do some interesting processing here to transform pkt
    //copy the packet object into the slot
    for(size_t i=0; i < pkt.length; i++){
      #ifdef SAMPLEPROCESSOR_DEBUG
      DEBUG_PORT.print(pkt.data[i], HEX);DEBUG_PORT.print(F(" "));
      #endif
      pkt.data[i] = raw_pkt.data[i];
    } 
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.println();
    #endif
    pkt.length    = raw_pkt.length; //update length field
    pkt.timestamp = raw_pkt.timestamp;
    pkt.flags     = raw_pkt.flags;
    return SUCCESS;
  }
  else if (status == ERROR_QUEUE_UNDERFLOW){
    return NO_PACKET_READY;
  }
  else{
    return status;
  }
}


void SampleProcessor::_put_at(size_t index, Packet& pkt)
{
  Packet *pkt_slot = &(_slots[index]); //pull out the slot by address
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::_put_at"));
  DEBUG_PORT.print(F("# \tindex="));DEBUG_PORT.println(index);
  DEBUG_PORT.print(F("# \t&pkt="));DEBUG_PORT.println((unsigned int) &pkt,HEX);
  DEBUG_PORT.print(F("# \tpkt_slot="));DEBUG_PORT.println((unsigned int) pkt_slot,HEX);
  DEBUG_PORT.print(F("# \tcopying data: "));
  #endif
  //copy the packet object into the slot
  for(size_t i=0; i < pkt.length; i++){
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.print(pkt.data[i], HEX);DEBUG_PORT.print(F(" "));
    #endif
    pkt_slot->data[i] = pkt.data[i];
  }
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println();
  #endif
  pkt_slot->length    = pkt.length; //update length field
  pkt_slot->timestamp = pkt.timestamp;
  pkt_slot->flags     = pkt.flags;
}

void SampleProcessor::_get_from(size_t index, Packet& pkt)
{
  Packet *pkt_slot = &(_slots[index]); //pull out the slot by address
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println(F("# In SampleProcessor::_get_from"));
  DEBUG_PORT.print(F("# \tindex="));DEBUG_PORT.println(index);
  DEBUG_PORT.print(F("# \t&pkt="));DEBUG_PORT.println((unsigned int) &pkt,HEX);
  DEBUG_PORT.print(F("# \tpkt_slot="));DEBUG_PORT.println((unsigned int) pkt_slot,HEX);
  DEBUG_PORT.print(F("# \tpkt_slot->length="));DEBUG_PORT.println(pkt_slot->length);
  DEBUG_PORT.print(F("# \tcopying data: 0x"));
  #endif
  //copy the slot data to the current the referenced packet object
  for(size_t i=0; (i < pkt_slot->length) && (i < _dataBufferSize); i++){
    #ifdef SAMPLEPROCESSOR_DEBUG
    DEBUG_PORT.print(pkt_slot->data[i], HEX);DEBUG_PORT.print(F(" "));
    #endif
    pkt.data[i] = pkt_slot->data[i];
  }
  #ifdef SAMPLEPROCESSOR_DEBUG
  DEBUG_PORT.println();
  #endif
  pkt.length    = min(pkt_slot->length,_dataBufferSize);
  pkt.timestamp = pkt_slot->timestamp;
  pkt.flags     = pkt_slot->flags;
}
