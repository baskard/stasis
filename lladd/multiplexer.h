#include "iterator.h"
#include "consumer.h"
#include <pbl/pbl.h>
/**
     A multiplexer takes an iterator, and splits its output into multiple consumers.
*/

typedef struct {  
  lladdIterator_t *iterator;
  lladdConsumer_t *consumer;
} lladdFifo_t;

typedef struct lladdFifoPool_t { 
  lladdFifo_t ** pool;
  lladdConsumer_t * (*getConsumer)(struct lladdFifoPool_t * pool, 
			      byte * multiplexKey, 
			      size_t multiplexKeySize);
  int fifoCount;
} lladdFifoPool_t;


void multiplexHashLogByKey(byte * key,
			   size_t keySize, 
			   byte * value, 
			   size_t valueSize, 
			   byte ** multiplexKey,
			   size_t * multiplexKeySize);

typedef struct  { 
  lladdIterator_t * it;
  void (*multiplexer)(byte * key, 
		      size_t keySize, 
		      byte * value, 
		      size_t valueSize, 
		      byte ** multiplexKey, 
		      size_t * multiplexKeySize);

  /** A hash of consumer implementations, keyed on the output of the multiplexKey parameter of *multiplex */
  pblHashTable_t * consumerHash;
  /** The next two fields are used to create new consumers on demand. */
  lladdConsumer_t * (*getConsumer)(struct lladdFifoPool_t *  newConsumerArg,
				   byte*  multiplexKey, 
				   size_t multiplexKeySize);
  lladdFifoPool_t * getConsumerArg;
  pthread_t worker;
  int xid;
} lladdMultiplexer_t;


lladdMultiplexer_t * lladdMultiplexer_alloc(int xid, lladdIterator_t * it, 
					    void (*multiplexer)(byte * key,
							      size_t keySize, 
							      byte * value, 
							      size_t valueSize, 
							      byte ** multiplexKey,
							      size_t * multiplexKeySize),
					    lladdConsumer_t * getConsumer(lladdFifoPool_t * getConsumerArg,
									  byte* multiplexKey, 
									  size_t multiplexKeySize),
					    lladdFifoPool_t * fifoPool);

/** 
    creates a new thread that will consume input from it, and forward
    its output to the consumers.  

    @param thread_attributes passed through to pthread_create, it is
    fine if this is NULL, although it probably makes sense to set the
    stack size to something reasonable (PTHREAD_STACK_MIN will
    probably work.  LLADD is tested with 16K stacks under linux/x86
    (where 16K = PTHREAD_STACK_MIN) , while the default pthread stack
    size is 2M.  Your milage may vary.)

    @return zero on success, or error code (@see pthread_create for
    possible return values) on failure.
*/
int lladdMultiplexer_start(lladdMultiplexer_t * multiplexer, pthread_attr_t * thread_attributes);

/**
    block the current thread until the multiplexer shuts down.

    @todo lladdMultiplex_join does not propagate compensation errors as it should.
 */
int lladdMultiplexer_join(lladdMultiplexer_t * multiplexer);

lladdConsumer_t * fifoPool_getConsumerCRC32( lladdFifoPool_t * pool, byte * multiplexKey, size_t multiplexKeySize);
lladdFifoPool_t * fifoPool_ringBufferInit (int consumerCount, int bufferSize);