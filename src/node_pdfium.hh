#ifndef NODE_PDFIUM_HPP
#define NODE_PDFIUM_HPP

#pragma once

#include <node.h>
#include <v8.h>

#include "macros.hh"


/**
 * Render PDF data to a different format
 *
 * @param options data Object, mandatory, options to convert
 * 	data - Binary, mandatory - binary data to be converted
 * 	outputFormat - String, mandatory - output format. A valid value from getSupportedOutputFormats() (PNG, EMF, BMP, etc)
 *  scaleFactor - Number, optional - scale factor
 * @param callback - Function, optional - if function is present then the method will be called asynchronous, otherwise synchronous.	
 * @return Array of Buffers - each item is the page in the same order
 */
MY_NODE_MODULE_CALLBACK(render);

/**
 * get all output supported formats
 * @return Array of strings with all supported output formats. e.g.: ['BMP', 'EMF', 'PNG', 'PPM']
 */
MY_NODE_MODULE_CALLBACK(getSupportedOutputFormats);


// util class

/** Memory value class management to avoid memory leak
 * TODO: move to std::unique_ptr on switching to C++11
*/
template<typename Type>
class MemValueBase
{
public:
    MemValueBase(): _value(NULL) {}

    /** Destructor. The allocated memory will be deallocated
    */
    ~MemValueBase() {
        free();
    }

    Type * get() {return _value; }
    Type * operator ->() { return _value; }
    operator bool() const { return (_value != NULL); }
    void set(Type * iValue) {
    	free();
    	_value = iValue;
    }
    Type * release() {
    	Type * tmp = _value;
    	_value = NULL;
    	return tmp;
    }
protected:
    Type *_value;

    virtual void free() {
    	if(_value != NULL) {
    		delete _value;
    		_value = NULL;
    	}
    };
};

#endif
