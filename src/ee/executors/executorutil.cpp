/* This file is part of VoltDB.
 * Copyright (C) 2008-2016 VoltDB Inc.
 *
 * This file contains original code and/or modifications of original code.
 * Any modifications made by VoltDB Inc. are licensed under the following
 * terms and conditions:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with VoltDB.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Copyright (C) 2008 by H-Store Project
 * Brown University
 * Massachusetts Institute of Technology
 * Yale University
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "executorutil.h"

#include "common/tabletuple.h"
#include "expressions/abstractexpression.h"
#include "storage/table.h"

namespace voltdb {

CountingPostfilter::CountingPostfilter(const Table* table, const AbstractExpression * postfilter, int limit, int offset,
    CountingPostfilter* parentPostfilter) :
    m_table(table),
    m_postfilter(postfilter),
    m_parentPostfilter(parentPostfilter),
    m_limit(limit),
    m_offset(offset),
    m_tuple_skipped(0),
    m_under_limit(true)
{}

CountingPostfilter::CountingPostfilter() :
    m_table(NULL),
    m_postfilter(NULL),
    m_parentPostfilter(NULL),
    m_limit(NO_LIMIT),
    m_offset(NO_OFFSET),
    m_tuple_skipped(0),
    m_under_limit(false)
{}

// Returns true if predicate evaluates to true and LIMIT/OFFSET conditions are satisfied.
bool CountingPostfilter::eval(const TableTuple* outer_tuple, const TableTuple* inner_tuple) {
    if (m_postfilter == NULL || m_postfilter->eval(outer_tuple, inner_tuple).isTrue()) {
        // Check if we have to skip this tuple because of offset
        if (m_tuple_skipped < m_offset) {
            m_tuple_skipped++;
            return false;
        }
        // Evaluate LIMIT now
        if (m_limit >= 0) {
            assert(m_table != NULL);
            if (m_table->activeTupleCount() == m_limit) {
                m_under_limit = false;
                // Notify a parent that the limit is reached
                if (m_parentPostfilter) {
                    m_parentPostfilter->setAboveLimit();
                }
                return false;
            }
        }
        // LIMIT/OFFSET are satisfied
        return true;
    }
    // Predicate is not NULL and was evaluated to FALSE
    return false;
}

}
