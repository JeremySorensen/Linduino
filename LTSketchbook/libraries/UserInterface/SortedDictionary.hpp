/*!
SortedDictionary: Stores key/values pairs in order by key and retrieves them.
Inserting and getting are both O(log N) (binary search). Currently removal is
not implemented, as the main use case for this class doesn't need it.

Copyright (c) 2018, Analog Devices Corp.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of Analog Devices Corp.

The Analog Devices Linduino is not affiliated with the official Arduino team.
However, the Linduino is only possible because of the Arduino team's commitment
to the open-source community.  Please visit http://www.arduino.cc and
http://store.arduino.cc and consider a purchase that will help fund their
ongoing work.

*/

#ifndef SORTED_DICTIONARY
#define SORTED_DICTIONARY

//! Class to store key/value pairs
//! @tparam K the type of the keys
//! @tparam KComp a class that has a function called compare used to compare keys
//! @tparam V type of the values
//! @tparam Capacity the maximimum number of key/value pairs that can be stored
template <class K, class KComp, class V, int Capacity>
class SortedDictionary {
    K keys[Capacity];
    V values[Capacity];
    int num_entries = 0;

    int search(K key, bool exact)
    {
        if (num_entries == 0) { return 0; }

        int lo = 0;
        int hi = num_entries - 1;

        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            int comp = KComp::compare(keys[mid], key);
            if (comp < 0) {
                lo = mid + 1;
            } else if (comp > 0) {
                hi = mid - 1;
            } else {
                if (exact) {
                return mid;
                } else {
                    return -1;
                }
            }
        }
        if (exact) {
            return -1;
        } else {
            return hi + 1;
        }
    }

    bool array_insert(int index, K key, V value)
    {
        if (num_entries == Capacity) {
            return false;
        } else if (keys[index] == key) {
            return false;
        }

        for (int i = num_entries; i > index; --i) {
            keys[i] = keys[i - 1];
            values[i] = values[i - 1];
        }
        
        keys[index] = key;
        values[index] = value;
        ++num_entries;
        return true;
    }

public:
    //! Inserts a key/value pair
    //! @return true if succeeded, false if too many elements or key exists
    bool insert(
        K key,  //!< Key to insert
        V value //!< Value to insert
        )
    {
        int index = search(key, false);
        return array_insert(index, key, value);
    }

    //! Get a value by key
    //! @return true if succeeded, false if key does not exist
    bool get_value(
        K key,   //! The key to look up
        V& value //! The value found
        )
    {
        int index = search(key, true);
        if (index < 0 || index >= num_entries) {
            return false;
        }
        value = values[index];
        return true;
    }

    //! Get the array of keys
    //! @return a pointer to the array of keys (don't modify it!)
    const K* get_keys()
    {
        return keys;
    }

    //! Get the array of values
    //! @return a pointer to the array of values (don't modify it!)
    const V* get_values()
    {
        return values;
    }

    //! get the number of entries (key/value pairs)
    //! @return the number of entries
    int get_num_entries()
    {
        return num_entries;
    }
};

#endif // SORTED_DICTIONARY
