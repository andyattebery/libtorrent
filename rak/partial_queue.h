// rak - Rakshasa's toolbox
// Copyright (C) 2005-2006, Jari Sundell
//
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
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RAK_PARTIAL_QUEUE_H
#define RAK_PARTIAL_QUEUE_H

#include <cstring>
#include <stdexcept>
#include <inttypes.h>

namespace rak {

// First step, don't allow overflowing to the next layer. Only disable
// the above layers for now.

// We also include 0 in a single layer as some chunk may be available
// only through seeders.

class partial_queue {
public:
  typedef uint8_t                         key_type;
  typedef uint32_t                        mapped_type;
  typedef uint16_t                        size_type;
  typedef std::pair<size_type, size_type> size_pair_type;

  static const size_type num_layers = 9;

  partial_queue() : m_data(NULL), m_maxLayerSize(0) {}
  ~partial_queue() { delete [] m_data; }

  bool                is_full() const                         { return is_layer_full(0); }
  bool                is_layer_full(size_type l) const        { return m_layers[l].second >= m_maxLayerSize; }

  // Add check to see if we can add more. Also make it possible to
  // check how full we are in the lower parts so the caller knows when
  // he can stop searching.
  //
  // Though propably not needed, as we must continue til the first
  // layer is full.

  size_type           max_size() const                        { return m_maxLayerSize * num_layers; }
  size_type           max_layer_size() const                  { return m_maxLayerSize; }

  // Must be less that or equal to (max size_type) / 8.
  void                resize(size_type ls);
  void                clear();

  // Safe to call while pop'ing and it will not reuse pop'ed indices
  // so it is guaranteed to reach max_size at some point. This will
  // ensure that the user needs to refill with new data at regular
  // intervals.
  bool                insert(key_type key, mapped_type value);

  // Only call this when pop'ing as it moves the index.
  bool                prepare_pop();

  mapped_type         pop();

private:
  partial_queue(const partial_queue&);
  void operator = (const partial_queue&);

  void                find_non_empty();

  mapped_type*        m_data;
  size_type           m_maxLayerSize;

  size_type           m_index;
  size_pair_type      m_layers[num_layers];
};

inline void
partial_queue::resize(size_type ls) {
  if (ls == m_maxLayerSize)
    return;

  delete m_data;
  m_maxLayerSize = ls;

  if (m_maxLayerSize == 0)
    return;

  m_data = new mapped_type[m_maxLayerSize * num_layers];
}

inline void
partial_queue::clear() {
  if (m_data == NULL)
    return;

  m_index = 0;
  std::memset(m_layers, 0, num_layers * sizeof(size_pair_type));
}

inline bool
partial_queue::insert(key_type key, mapped_type value) {
  size_type idx = 0;

  for (; key >= (2 << idx) - 1; ++idx)
    if (is_layer_full(idx))
      return false;

  m_index = std::min(m_index, idx);

  // Currently don't allow overflow.
  if (is_layer_full(idx))
    return false;

  m_data[m_maxLayerSize * idx + m_layers[idx].second] = value;
  m_layers[idx].second++;

  return true;
}

// is_empty() will iterate to the first layer with un-popped elements
// and return true, else return false when it reaches a overflowed or
// the last layer.
inline bool
partial_queue::prepare_pop() {
  while (m_layers[m_index].first == m_layers[m_index].second) {
    if (is_layer_full(m_index) || m_index + 1 == num_layers)
      return false;

    m_index++;
  }

  return true;
}

inline partial_queue::mapped_type
partial_queue::pop() {
  if (m_index >= num_layers || m_layers[m_index].first >= m_layers[m_index].second)
    throw std::logic_error("partial_queue::pop() bad state.");

  return m_data[m_index * m_maxLayerSize + m_layers[m_index].first++];
}

}

#endif