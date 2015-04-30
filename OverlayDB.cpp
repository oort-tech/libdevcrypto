/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file OverlayDB.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <libdevcore/Common.h>
#include "OverlayDB.h"
using namespace std;
using namespace dev;

namespace dev
{

OverlayDB::~OverlayDB()
{
	if (m_db.use_count() == 1 && m_db.get())
		cnote << "Closing state DB";
}

void OverlayDB::commit()
{
	if (m_db)
	{
		ldb::WriteBatch batch;
//		cnote << "Committing nodes to disk DB:";
		for (auto const& i: m_over)
		{
//			cnote << i.first << "#" << m_refCount[i.first];
			if (m_refCount[i.first])
				batch.Put(ldb::Slice((char const*)i.first.data(), i.first.size), ldb::Slice(i.second.data(), i.second.size()));
		}
		for (auto const& i: m_auxActive)
			if (m_aux.count(i))
			{
				batch.Put(i.ref(), bytesConstRef(&m_aux[i]));
				m_aux.erase(i);
			}
		m_db->Write(m_writeOptions, &batch);
		m_auxActive.clear();
		m_aux.clear();
		m_over.clear();
		m_refCount.clear();
	}
}

bytes OverlayDB::lookupAux(h256 _h) const
{
	bytes ret = MemoryDB::lookupAux(_h);
	if (!ret.empty())
		return ret;
	std::string v;
	m_db->Get(m_readOptions, aux(_h).ref(), &v);
	if (v.empty())
		cwarn << "Aux not found: " << _h;
	return asBytes(v);
}

void OverlayDB::rollback()
{
	m_over.clear();
	m_refCount.clear();
}

std::string OverlayDB::lookup(h256 _h) const
{
	std::string ret = MemoryDB::lookup(_h);
	if (ret.empty() && m_db)
		m_db->Get(m_readOptions, ldb::Slice((char const*)_h.data(), 32), &ret);
	return ret;
}

bool OverlayDB::exists(h256 _h) const
{
	if (MemoryDB::exists(_h))
		return true;
	std::string ret;
	if (m_db)
		m_db->Get(m_readOptions, ldb::Slice((char const*)_h.data(), 32), &ret);
	return !ret.empty();
}

void OverlayDB::kill(h256 _h)
{
#if ETH_PARANOIA
	if (!MemoryDB::kill(_h))
	{
		std::string ret;
		if (m_db)
			m_db->Get(m_readOptions, ldb::Slice((char const*)_h.data(), 32), &ret);
		if (ret.empty())
			cnote << "Decreasing DB node ref count below zero with no DB node. Probably have a corrupt Trie." << _h;
	}
#else
	MemoryDB::kill(_h);
#endif
}

}
