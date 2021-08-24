#ifndef TO_h
#define TO_h

#include <stdint.h>

// The maximum number of parameters available to track in the telemetry system.
// This value can be redefined to increase the parameters but be careful not
// exceed the maximum packet size of the underlying communication mechanism.
#define TO_MAX_PARAMS 16

/*
 * TO_Object_t indicates a telemetry parameter and it's associated value.
 * 
 * Parmeters are mission specific and can be any uint8. The sender and receiver
 * must agree on what they mean and how to interpret the value.
 * 
 * Each value is a uint32. That's all you get.
 */
typedef struct {
    uint8_t  param;
    uint32_t data;
} TO_Object_t;

// On the wire, this is how large an objects should be.
#define TO_OBJECT_SIZE 5

/**
 * TO_t Telemetry Output component tracks multiple parameters and provides
 * interfaces for encoding them for transmission.
 */
typedef struct {
    TO_Object_t objects[TO_MAX_PARAMS];
} TO_t;

/*
 * TO_init initializes the TO_t struct for use.
 * 
 * @param to Pointer to TO_t component.
 */
int TO_init(TO_t *to);

/*
 * TO_init initializes the TO_t struct for use.
 * 
 * @param to Pointer to TO_t component.
 * @param param Parameter to set
 * @param value Value for parameter
 * 
 * @returns 0 on success
 * @returns -1 if the parameter doesn't fit within TO_t
 */
int TO_set(TO_t *to, uint8_t param, uint32_t value);

/*
 * TO_encode encodes the collected telemetry data into the provided buffer.
 * 
 * If the provided data buffer is too small, only those parameters that fit
 * will be encoded.
 * 
 * @param to Pointer to TO_t component.
 * @param buf Pointer to data buffer for encoded data.
 * @param size size of data buffer
 * 
 * @returns size of encoded telemetry data for use in sending through a communication channel.
 */
size_t TO_encode(TO_t *to, uint8_t *buf, size_t size);

/*
 * TO_decode decodes the telemetry buffer into a TO_t struct.
 * 
 * @param to Pointer to TO_t component.
 * @param buf Pointer to data buffer
 * @param size size of data buffer
 * 
 * @returns count of decoded objects
 */
int TO_decode(TO_t *to, uint8_t *buf, size_t size);

#endif