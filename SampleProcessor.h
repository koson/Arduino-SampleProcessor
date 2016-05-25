/*  
*/
#ifndef _SAMPLE_PROCESSOR_H_INCLUDED
#define _SAMPLE_PROCESSOR_H_INCLUDED

#include <Arduino.h>
#include <stdint.h>

//uncomment for debugging
//#define SAMPLEPROCESSOR_DEBUG

#ifndef DEBUG_PORT
  #define DEBUG_PORT Serial
#endif

class SampleProcessor
{
public:
  static const size_t DATA_BUFFER_SIZE = 32;
  
  // Status and Error  Codes
  typedef enum StatusCode {
    NO_PACKET_READY             = 1,
    SUCCESS                     = 0,
    ERROR_INVALID_PACKET        = -3,
    ERROR_PACKET_INDEX_OUT_OF_BOUNDS = -7,
    ERROR_INPUT_BUFFER_OVERRUN   = -8,
    ERROR_QUEUE_OVERFLOW         = -9,
    ERROR_QUEUE_UNDERFLOW        = -10,
    ERROR_MEMALLOC_FAIL          = -11
  } STATUS;

  // Packet structure
  struct Packet {
    byte     data[DATA_BUFFER_SIZE];
    size_t   length;
    uint32_t timestamp;
    byte     flags;
  };

  SampleProcessor();
  STATUS begin(size_t capacity);
  STATUS end();
  size_t size() const { return _size; }
  size_t capacity() const { return _capacity; }
  STATUS reset();
  STATUS enqueue_rawdata(Packet& pkt);
  STATUS dequeue_rawdata(Packet& pkt);
  STATUS requeue_rawdata(Packet& pkt);
  STATUS process(Packet& pkt);
  // Empty the queue,return number of packets flushed
  size_t flush();

private:
  
  void _put_at(size_t index, Packet& pkt);
  void _get_from(size_t index, Packet& pkt);
  
  size_t _beg_index;
  size_t _end_index;
  size_t _size; 
  size_t _capacity;
  size_t _dataBufferSize;
  
  Packet* _slots;
  
};

#endif /* _SAMPLE_PROCESSOR_H_INCLUDED */
