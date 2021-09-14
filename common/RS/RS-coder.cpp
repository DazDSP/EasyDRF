/*
(**************************************************************************)
(*                                                                        *)
(*                                Schifra                                 *)
(*                Reed-Solomon Error Correcting Code Library              *)
(*                                                                        *)
(* Release Version 0.0.1                                                  *)
(* http://www.schifra.com                                                 *)
(* Copyright (c) 2000-2020 Arash Partow, All Rights Reserved.             *)
(*                                                                        *)
(* The Schifra Reed-Solomon error correcting code library and all its     *)
(* components are supplied under the terms of the General Schifra License *)
(* agreement. The contents of the Schifra Reed-Solomon error correcting   *)
(* code library and all its components may not be copied or disclosed     *)
(* except in accordance with the terms of that agreement.                 *)
(*                                                                        *)
(* URL: http://www.schifra.com/license.html                               *)
(*                                                                        *)
(**************************************************************************)
*/

//This file assembled from example code and customised for EasyDRF by Daz Man 2021

#include <cstddef>
#include <iostream>
#include <string>

#include "schifra_galois_field.hpp"
#include "schifra_galois_field_polynomial.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_decoder.hpp"
#include "schifra_reed_solomon_block.hpp"
#include "schifra_error_processes.hpp"
//#include "../../RS-defs.h"

#define RScodeLength 255;
#define RS1dataLength 224;
#define RS2dataLength 192;
#define RS3dataLength 160;
#define RS4dataLength 128;

//====================================================================================================================================
// Encoders
//====================================================================================================================================

int rs1encode(unsigned char* inbuf, unsigned char* outbuf, unsigned int filesize) {
    /* Reed Solomon Code Parameters */
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS1dataLength;
    const std::size_t fec_length = code_length - data_length;

    /* Finite Field Parameters */
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    /* Instantiate Finite Field and Generator Polynomials */
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    /* Instantiate Encoder */
    typedef schifra::reed_solomon::encoder<code_length, fec_length, data_length> encoder_t;
    const encoder_t encoder(field, generator_polynomial);

    /* Instantiate RS Block For Codec */
    schifra::reed_solomon::block<code_length, fec_length> block;

    //loop through buffer and read data in blocks of data_length bytes to feed it, until done to length DM
    int i = filesize % data_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / data_length) + i; //round up to nearest multiple of data_length
    int k = 0;
    i = 0;

    std::string message;
    message.resize(code_length, 0x00);

    while (times > 0) {
        int j = 0;
        //read in data_length x 8-bit symbols of data to be encoded
        while (j < data_length) {
            message[j] = inbuf[i]; //read from inbuf, and write to message buffer, data_length symbols
            j++;
            i++;
        }
        /* Pad message with nulls up until the code-word length */
        //message.resize(code_length, 0x00);

        if (!encoder.encode(message, block)) { //encode from message buffer, into block buffer
            return 1; //error code if failed
        }
        else {
            int j = 0;
            while (j < code_length) {
                outbuf[k] = block[j]; //read out 255 encoded bytes to outbuf
                k++;
                j++;
            }
        }
        times--;
    }
    return 0;
}

int rs2encode(unsigned char* inbuf, unsigned char* outbuf, unsigned int filesize) {
    /* Reed Solomon Code Parameters */
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS2dataLength;
    const std::size_t fec_length = code_length - data_length;

    /* Finite Field Parameters */
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    /* Instantiate Finite Field and Generator Polynomials */
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    /* Instantiate Encoder */
    typedef schifra::reed_solomon::encoder<code_length, fec_length, data_length> encoder_t;
    const encoder_t encoder(field, generator_polynomial);

    /* Instantiate RS Block For Codec */
    schifra::reed_solomon::block<code_length, fec_length> block;

    //loop through buffer and read data in blocks of data_length bytes to feed it, until done to length DM
    int i = filesize % data_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / data_length)+i; //round up to nearest multiple of data_length
    int k = 0;
    i = 0;

    std::string message;
    message.resize(code_length, 0x00);

    while (times > 0) {
       int j = 0;
        //read in data_length x 8-bit symbols of data to be encoded
        while (j < data_length) {
            message[j] = inbuf[i]; //read from inbuf, and write to message buffer, data_length symbols
            j++;
            i++;
        }
        /* Pad message with nulls up until the code-word length */
        //message.resize(code_length, 0x00);

        if (!encoder.encode(message, block)) { //encode from message buffer, into block buffer
            return 1; //error code if failed
        }
        else {
            int j = 0;
            while (j < code_length) {
                outbuf[k] = block[j]; //read out 255 encoded bytes to outbuf
                k++;
                j++;
            }
        }
        times--;
    }
    return 0;
}

int rs3encode(unsigned char* inbuf, unsigned char* outbuf, unsigned int filesize) {
    /* Reed Solomon Code Parameters */
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS3dataLength;
    const std::size_t fec_length = code_length - data_length;

    /* Finite Field Parameters */
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    /* Instantiate Finite Field and Generator Polynomials */
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    /* Instantiate Encoder */
    typedef schifra::reed_solomon::encoder<code_length, fec_length, data_length> encoder_t;
    const encoder_t encoder(field, generator_polynomial);

    /* Instantiate RS Block For Codec */
    schifra::reed_solomon::block<code_length, fec_length> block;

    //loop through buffer and read data in blocks of data_length bytes to feed it, until done to length DM
    int i = filesize % data_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / data_length)+i; //round up to nearest multiple of data_length
    int k = 0;
    i = 0;

    std::string message;
    message.resize(code_length, 0x00);

    while (times > 0) {
       int j = 0;
        //read in data_length x 8-bit symbols of data to be encoded
        while (j < data_length) {
            message[j] = inbuf[i]; //read from inbuf, and write to message buffer, data_length symbols
            j++;
            i++;
        }
        /* Pad message with nulls up until the code-word length */
        //message.resize(code_length, 0x00);

        if (!encoder.encode(message, block)) { //encode from message buffer, into block buffer
            return 1; //error code if failed
        }
        else {
            int j = 0;
            while (j < code_length) {
                outbuf[k] = block[j]; //read out 255 encoded bytes to outbuf
                k++;
                j++;
            }
        }
        times--;
    }
    return 0;
}

int rs4encode(unsigned char* inbuf, unsigned char* outbuf, unsigned int filesize) {
    /* Reed Solomon Code Parameters */
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS4dataLength;
    const std::size_t fec_length = code_length - data_length;

    /* Finite Field Parameters */
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    /* Instantiate Finite Field and Generator Polynomials */
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    /* Instantiate Encoder */
    typedef schifra::reed_solomon::encoder<code_length, fec_length, data_length> encoder_t;
    const encoder_t encoder(field, generator_polynomial);

    /* Instantiate RS Block For Codec */
    schifra::reed_solomon::block<code_length, fec_length> block;

    //loop through buffer and read data in blocks of data_length bytes to feed it, until done to length DM
    int i = filesize % data_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / data_length)+i; //round up to nearest multiple of data_length
    int k = 0;
    i = 0;

    std::string message;
    message.resize(code_length, 0x00);

    while (times > 0) {
       int j = 0;
        //read in data_length x 8-bit symbols of data to be encoded
        while (j < data_length) {
            message[j] = inbuf[i]; //read from inbuf, and write to message buffer, data_length symbols
            j++;
            i++;
        }
        /* Pad message with nulls up until the code-word length */
        //message.resize(code_length, 0x00);

        if (!encoder.encode(message, block)) { //encode from message buffer, into block buffer
            return 1; //error code if failed
        }
        else {
            int j = 0;
            while (j < code_length) {
                outbuf[k] = block[j]; //read out 255 encoded bytes to outbuf
                k++;
                j++;
            }
        }
        times--;
    }
    return 0;
}

//====================================================================================================================================
// Decoders
//====================================================================================================================================

//RS1 decoder with erasure processing DM
int rs1decodeE(unsigned char* inbuf, unsigned char* inbuf2, unsigned  char* outbuf, unsigned int filesize) {
    // Reed Solomon Code Parameters
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS1dataLength;
    const std::size_t fec_length = code_length - data_length;
    const std::size_t stack_size = 1;

    // Finite Field Parameters
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    // Instantiate Finite Field and Generator Polynomials
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    // Instantiate Decoder 
    typedef schifra::reed_solomon::decoder<code_length, fec_length, data_length> decoder_t;
    const decoder_t decoder(field, generator_polynomial_index);

    // Instantiate RS Block For Codec
    schifra::reed_solomon::block<code_length, fec_length> block;

    //the erasure array
    schifra::reed_solomon::erasure_locations_t erasure_location_list;
    erasure_location_list.clear();

    if (code_length == 0) { return 5; } //div by 0 prevention

    //loop through buffer and read data in blocks of 255 bytes to feed it, until done to length DM
    int i = filesize % code_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / code_length) + i; //round up to nearest multiple of code_length
    int k = 0;
    i = 0;
    int errors = 0;
  
    int bc = 0; //block count

    while (bc < times) {
        int j = 0;
        erasure_location_list.clear();
        //read in 255 x 8-bit symbols of data to be decoded
        while (j < code_length) {
            block[j] = inbuf[i]; //read data from inbuf, and write to message buffer

            //inbuf2 is already unpacked and deinterleaved
            //0 = CRC error
            //1 = CRC OK
            //check for erasures at buffer index i, and set an entry for block index j
            if (inbuf2[i] == 0) {
                erasure_location_list.push_back(j);
            }

            j++;
            i++;
        }

        if (!decoder.decode(block, erasure_location_list)){
          errors++;
            //replace data by zeroes
            int j = 0;
            while (j < data_length) {
                outbuf[k] = 0; //read out data_length x 0's
                k++;
                j++;
            }
        }
        else {
            int j = 0;
            while (j < data_length) {
                outbuf[k] = block[j]; //read out data_length decoded bytes to outbuf
                k++;
                j++;
            }
        }
//        times--;
           bc++;
    }
    return errors;
}

//RS2 decoder with erasure processing DM
int rs2decodeE(unsigned char* inbuf, unsigned char* inbuf2, unsigned  char* outbuf, unsigned int filesize) {
    // Reed Solomon Code Parameters
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS2dataLength;
    const std::size_t fec_length = code_length - data_length;
    const std::size_t stack_size = 1;

    // Finite Field Parameters
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    // Instantiate Finite Field and Generator Polynomials
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    // Instantiate Decoder 
    typedef schifra::reed_solomon::decoder<code_length, fec_length, data_length> decoder_t;
    const decoder_t decoder(field, generator_polynomial_index);

    // Instantiate RS Block For Codec
    schifra::reed_solomon::block<code_length, fec_length> block;

    //the erasure array
    schifra::reed_solomon::erasure_locations_t erasure_location_list;
    erasure_location_list.clear();

    if (code_length == 0) { return 5; } //div by 0 prevention

    //loop through buffer and read data in blocks of 255 bytes to feed it, until done to length DM
    int i = filesize % code_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / code_length) + i; //round up to nearest multiple of code_length
    int k = 0;
    i = 0;
    int errors = 0;
  
    int bc = 0; //block count

    while (bc < times) {
        int j = 0;
        erasure_location_list.clear();
        //read in 255 x 8-bit symbols of data to be decoded
        while (j < code_length) {
            block[j] = inbuf[i]; //read data from inbuf, and write to message buffer

            //inbuf2 is already unpacked and deinterleaved
            //0 = CRC error
            //1 = CRC OK
            //check for erasures at buffer index i, and set an entry for block index j
            if (inbuf2[i] == 0) {
                erasure_location_list.push_back(j);
            }

            j++;
            i++;
        }

        if (!decoder.decode(block, erasure_location_list)){
          errors++;
            //replace data by zeroes
            int j = 0;
            while (j < data_length) {
                outbuf[k] = 0; //read out data_length x 0's
                k++;
                j++;
            }
        }
        else {
            int j = 0;
            while (j < data_length) {
                outbuf[k] = block[j]; //read out data_length decoded bytes to outbuf
                k++;
                j++;
            }
        }
//        times--;
           bc++;
    }
    return errors;
}

//RS3 decoder with erasure processing DM
int rs3decodeE(unsigned char* inbuf, unsigned char* inbuf2, unsigned  char* outbuf, unsigned int filesize) {
    // Reed Solomon Code Parameters
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS3dataLength;
    const std::size_t fec_length = code_length - data_length;
    const std::size_t stack_size = 1;

    // Finite Field Parameters
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    // Instantiate Finite Field and Generator Polynomials
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    // Instantiate Decoder 
    typedef schifra::reed_solomon::decoder<code_length, fec_length, data_length> decoder_t;
    const decoder_t decoder(field, generator_polynomial_index);

    // Instantiate RS Block For Codec
    schifra::reed_solomon::block<code_length, fec_length> block;

    //the erasure array
    schifra::reed_solomon::erasure_locations_t erasure_location_list;
    erasure_location_list.clear();

    if (code_length == 0) { return 5; } //div by 0 prevention

    //loop through buffer and read data in blocks of 255 bytes to feed it, until done to length DM
    int i = filesize % code_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / code_length) + i; //round up to nearest multiple of code_length
    int k = 0;
    i = 0;
    int errors = 0;
  
    int bc = 0; //block count

    while (bc < times) {
        int j = 0;
        erasure_location_list.clear();
        //read in 255 x 8-bit symbols of data to be decoded
        while (j < code_length) {
            block[j] = inbuf[i]; //read data from inbuf, and write to message buffer

            //inbuf2 is already unpacked and deinterleaved
            //0 = CRC error
            //1 = CRC OK
            //check for erasures at buffer index i, and set an entry for block index j
            if (inbuf2[i] == 0) {
                erasure_location_list.push_back(j);
            }

            j++;
            i++;
        }

        if (!decoder.decode(block, erasure_location_list)){
          errors++;
            //replace data by zeroes
            int j = 0;
            while (j < data_length) {
                outbuf[k] = 0; //read out data_length x 0's
                k++;
                j++;
            }
        }
        else {
            int j = 0;
            while (j < data_length) {
                outbuf[k] = block[j]; //read out data_length decoded bytes to outbuf
                k++;
                j++;
            }
        }
//        times--;
           bc++;
    }
    return errors;
}

//RS4 decoder with erasure processing DM
int rs4decodeE(unsigned char* inbuf, unsigned char* inbuf2, unsigned  char* outbuf, unsigned int filesize) {
    // Reed Solomon Code Parameters
    const std::size_t code_length = RScodeLength;
    const std::size_t data_length = RS4dataLength;
    const std::size_t fec_length = code_length - data_length;
    const std::size_t stack_size = 1;

    // Finite Field Parameters
    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = fec_length;

    // Instantiate Finite Field and Generator Polynomials
    const schifra::galois::field field(field_descriptor, schifra::galois::primitive_polynomial_size06, schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(field, generator_polynomial_index, generator_polynomial_root_count, generator_polynomial))
    {
        return 1;
    }

    // Instantiate Decoder 
    typedef schifra::reed_solomon::decoder<code_length, fec_length, data_length> decoder_t;
    const decoder_t decoder(field, generator_polynomial_index);

    // Instantiate RS Block For Codec
    schifra::reed_solomon::block<code_length, fec_length> block;

    //the erasure array
    schifra::reed_solomon::erasure_locations_t erasure_location_list;
    erasure_location_list.clear();

    if (code_length == 0) { return 5; } //div by 0 prevention

    //loop through buffer and read data in blocks of 255 bytes to feed it, until done to length DM
    int i = filesize % code_length; //if there is any remainder, decode one extra time
    if (i > 0) { i = 1; }
    int times = (filesize / code_length) + i; //round up to nearest multiple of code_length
    int k = 0;
    i = 0;
    int errors = 0;
  
    int bc = 0; //block count

    while (bc < times) {
        int j = 0;
        erasure_location_list.clear();
        //read in 255 x 8-bit symbols of data to be decoded
        while (j < code_length) {
            block[j] = inbuf[i]; //read data from inbuf, and write to message buffer

            //inbuf2 is already unpacked and deinterleaved
            //0 = CRC error
            //1 = CRC OK
            //check for erasures at buffer index i, and set an entry for block index j
            if (inbuf2[i] == 0) {
                erasure_location_list.push_back(j);
            }

            j++;
            i++;
        }

        if (!decoder.decode(block, erasure_location_list)){
          errors++;
            //replace data by zeroes
            int j = 0;
            while (j < data_length) {
                outbuf[k] = 0; //read out data_length x 0's
                k++;
                j++;
            }
        }
        else {
            int j = 0;
            while (j < data_length) {
                outbuf[k] = block[j]; //read out data_length decoded bytes to outbuf
                k++;
                j++;
            }
        }
//        times--;
           bc++;
    }
    return errors;
}


//This data interleaver/deinterleaver routine from QSSTV, heavily modified by Daz Man 2021
void distribute(unsigned char* src, unsigned char* dst, unsigned int filesize, bool reverse) {
    //filesize = RS encoded data size DM
    unsigned int rows = 255; //the block size of the RS coder is always 255
    unsigned int cols = (filesize / rows); //cols depend on the data size (how many 255 byte blocks)
    unsigned int i, j, rd;
    rd = 0;

    if (reverse == 0) {
        for (i = 0; i < filesize; i += rows)
        {
            for (j = 0; j < rows && ((i + j) < filesize); j++)
            {
                dst[rd] = src[i + j];
                rd += cols; /* next position in dst */
                if (rd >= filesize)
                {
                    rd -= filesize - 1;  /* go around and add one. */
                }
            }
        }
    }
    else {
        for (i = 0; i < filesize; i += rows)
        {
            for (j = 0; j < rows && ((i + j) < filesize); j++)
            {
                dst[i + j] = src[rd]; //reversed addressing from above
                rd += cols; /* next position in src */
                if (rd >= filesize)
                {
                    rd -= filesize - 1;  /* go around and add one. */
                }
            }
        }
    }
}

