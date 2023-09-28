#include <stddef.h>
#include <inttypes.h>

#define DATA_MAX (((256 + 3) / 4) * 4)
#define MSG_MAX 4096

typedef struct 
{

  int8_t    type;             // Type of message
  uint16_t  hash;             // Hash
  uint8_t   size;             // Length of data 
  char      data[DATA_MAX];   // Data of message

} message;

typedef struct 
{

  int       add_count;        // Amount of all added messages
  int       extract_count;    // Amount of all recieved messages
  int       msg_count;        // Number of msg that wait and will be recieved
  int       head;             // Number of msg that will be sent
  int       tail;             // Number of msg that is on the top
  message   buffer[MSG_MAX];  // Buffer of messages

} message_queue;