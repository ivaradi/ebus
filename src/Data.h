// Copyright (c) 2016 by István Váradi

// This file is part of eBUS, an eBUS handler utility

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef DATA_H
#define DATA_H
//------------------------------------------------------------------------------

#include "DataSymbolReader.h"

#include <cmath>

#include <inttypes.h>

//------------------------------------------------------------------------------

/**
 * Template for simple data types, where the raw data is an 8- or 16-bit
 * unsigned value, and the translated is a signed or unsigned integer of the
 * same length with not other conversion.
 */
template <typename rawT, typename valueT>
class SimpleData
{
public:
    /**
     * The raw value (in host byte order, where applicable).
     */
    rawT rawValue;

    /**
     * Construct with the given translated value.
     */
    SimpleData(valueT value);

    /**
     * Construct by reading data from the given reader.
     */
    SimpleData(DataSymbolReader& reader);

    /**
     * Get the translated value.
     */
    valueT get() const;

    /**
     * Get the translated value.
     */
    operator valueT() const;

    /**
     * Set the given translated value.
     */
    void set(valueT value);

    /**
     * Set the given translated value.
     */
    valueT operator=(valueT value);
};

//------------------------------------------------------------------------------

/**
 * A CHAR data item.
 */
typedef SimpleData<uint8_t, uint8_t> CharData;

//------------------------------------------------------------------------------

/**
 * A BYTE data item.
 */
typedef SimpleData<uint8_t, uint8_t> ByteData;

//------------------------------------------------------------------------------

/**
 * A BIT data item.
 */
typedef SimpleData<uint8_t, uint8_t> BitData;

//------------------------------------------------------------------------------

/**
 * A SIGNED CHAR data item.
 */
typedef SimpleData<uint8_t, int8_t> SignedCharData;

//------------------------------------------------------------------------------

/**
 * A SIGNED INTEGER data item.
 */
typedef SimpleData<uint16_t, int16_t> SignedIntData;

//------------------------------------------------------------------------------

/**
 * A WORD data item.
 */
typedef SimpleData<uint16_t, uint16_t> WordData;

//------------------------------------------------------------------------------

/**
 * A BCD data.
 */
class BCDData
{
public:
    /**
     * Convert the given BCD (raw) value into decimal.
     */
    static uint8_t fromBCD(uint8_t bcd);

    /**
     * Convert the given decimal value into a BCD one.
     */
    static uint8_t toBCD(uint8_t value);

    /**
     * The raw value.
     */
    uint8_t rawValue;

    /**
     * Construct with the given translated value.
     */
    BCDData(uint8_t value);

    /**
     * Construct by reading the value from the given data symbol reader.
     */
    BCDData(DataSymbolReader& reader);

    /**
     * Get the translated value.
     */
    uint8_t get() const;

    /**
     * Get the translated value.
     */
    operator uint8_t() const;

    /**
     * Set the given translated value.
     */
    void set(uint8_t value);

    /**
     * Set the given translated value.
     */
    uint8_t operator=(uint8_t value);
};

//------------------------------------------------------------------------------

/**
 * A DATA1b data.
 */
typedef SignedCharData Data1b;

//------------------------------------------------------------------------------

/**
 * Template for data types that produce a double value from an integer one by
 * dividing the integer value by some number.
 */
template <typename rawT, unsigned divisor, typename realRawT = rawT>
class DoubleData
{
public:
    /**
     * Convert the given Data1c (raw) value into the correponding translated
     * (real) value.
     */
    static double fromRaw(rawT rawValue);

    /**
     * Convert the given translated (real) value into a raw one.
     */
    static rawT toRaw(double value);

    /**
     * The raw value.
     */
    rawT rawValue;

    /**
     * Construct with the given translated value.
     */
    DoubleData(double value);

    /**
     * Construct by reading the value from the given data symbol reader.
     */
    DoubleData(DataSymbolReader& reader);

    /**
     * Get the translated value.
     */
    double get() const;

    /**
     * Get the translated value.
     */
    operator double() const;

    /**
     * Set the given translated value.
     */
    void set(double value);

    /**
     * Set the given translated value.
     */
    double operator=(double value);
};

//------------------------------------------------------------------------------

/**
 * A DATA1c data.
 */
typedef DoubleData<uint8_t, 2> Data1c;

//------------------------------------------------------------------------------

/**
 * A DATA2b data.
 */
typedef DoubleData<uint16_t, 256, int16_t> Data2b;

//------------------------------------------------------------------------------

/**
 * A DATA2c data.
 */
typedef DoubleData<uint16_t, 16, int16_t> Data2c;

//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

template <typename rawT, typename valueT>
inline SimpleData<rawT, valueT>::SimpleData(valueT value) :
    rawValue(static_cast<rawT>(value))
{
}

//------------------------------------------------------------------------------

template <typename rawT, typename valueT>
inline SimpleData<rawT, valueT>::SimpleData(DataSymbolReader& reader) :
    rawValue(reader)
{
}

//------------------------------------------------------------------------------

template <typename rawT, typename valueT>
inline valueT SimpleData<rawT, valueT>::get() const
{
    return static_cast<valueT>(rawValue);
}

//------------------------------------------------------------------------------

template <typename rawT, typename valueT>
inline SimpleData<rawT, valueT>::operator valueT() const
{
    return static_cast<valueT>(rawValue);
}

//------------------------------------------------------------------------------

template <typename rawT, typename valueT>
inline void SimpleData<rawT, valueT>::set(valueT value)
{
    rawValue = static_cast<rawT>(value);
}

//------------------------------------------------------------------------------

template <typename rawT, typename valueT>
inline valueT SimpleData<rawT, valueT>::operator=(valueT value)
{
    rawValue = static_cast<rawT>(value);
    return value;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline uint8_t BCDData::fromBCD(uint8_t bcd)
{
    return (bcd>>4)*10 + (bcd%16);
}

//------------------------------------------------------------------------------

inline uint8_t BCDData::toBCD(uint8_t value)
{
    return 16*((value/10)%10) + (value%10);
}

//------------------------------------------------------------------------------

inline BCDData::BCDData(uint8_t value) :
    rawValue(toBCD(value))
{
}

//------------------------------------------------------------------------------

inline BCDData::BCDData(DataSymbolReader& reader) :
    rawValue(reader)
{
}

//------------------------------------------------------------------------------

inline uint8_t BCDData::get() const
{
    return fromBCD(rawValue);
}

//------------------------------------------------------------------------------

inline BCDData::operator uint8_t() const
{
    return fromBCD(rawValue);
}

//------------------------------------------------------------------------------

inline void BCDData::set(uint8_t value)
{
    rawValue = toBCD(value);
}

//------------------------------------------------------------------------------

inline uint8_t BCDData::operator=(uint8_t value)
{
    rawValue = toBCD(value);
    return value;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline double DoubleData<rawT, divisor, realRawT>::fromRaw(rawT rawValue)
{
    return static_cast<realRawT>(rawValue) / static_cast<double>(divisor);
}

//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline rawT DoubleData<rawT, divisor, realRawT>::toRaw(double value)
{
    return static_cast<rawT>(round(value*divisor));
}

//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline DoubleData<rawT, divisor, realRawT>::DoubleData(double value) :
    rawValue(toRaw(value))
{
}

//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline DoubleData<rawT, divisor, realRawT>::DoubleData(DataSymbolReader& reader) :
    rawValue(reader)
{
}

//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline double DoubleData<rawT, divisor, realRawT>::get() const
{
    return fromRaw(rawValue);
}

//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline DoubleData<rawT, divisor, realRawT>::operator double() const
{
    return fromRaw(rawValue);
}

//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline void DoubleData<rawT, divisor, realRawT>::set(double value)
{
    rawValue = toRaw(value);
}

//------------------------------------------------------------------------------

template <typename rawT, unsigned divisor, typename realRawT>
inline double DoubleData<rawT, divisor, realRawT>::operator=(double value)
{
    rawValue = toRaw(value);
    return value;
}

//------------------------------------------------------------------------------
#endif // DATA_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
