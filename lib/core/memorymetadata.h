/**************************************************************************************
 * EMStudio - Open Source ECU tuning software                                         *
 * Copyright (C) 2013  Michael Carpenter (malcom2073@gmail.com)                       *
 *                                                                                    *
 * This file is a part of EMStudio                                                    *
 *                                                                                    *
 * EMStudio is free software; you can redistribute it and/or                          *
 * modify it under the terms of the GNU Lesser General Public                         *
 * License as published by the Free Software Foundation, version                      *
 * 2.1 of the License.                                                                *
 *                                                                                    *
 * EMStudio is distributed in the hope that it will be useful,                        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                  *
 * Lesser General Public License and GNU General Public License for                   *
 * more details.                                                                      *
 *                                                                                    *
 * You should have received a copy of the GNU Lesser General Public License           *
 * along with this program; if not, write to Free Software Foundation, Inc.,          *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA                       *
 *                                                                                    *
 *------------------------------------------------------------------------------------*
 *                                                                                    *
 * BSD 3-Clause License Usage                                                         *
 * Alternatively, this file may also be used under the terms of the                   *
 * BSD 3-Clause license, detailed below:                                              *
 *                                                                                    *
 * Redistribution and use in source and binary forms, with or without                 *
 * modification, are permitted provided that the following conditions are met:        *
 *     * Redistributions of source code must retain the above copyright               *
 *       notice, this list of conditions and the following disclaimer.                *
 *     * Redistributions in binary form must reproduce the above copyright            *
 *       notice, this list of conditions and the following disclaimer in the          *
 *       documentation and/or other materials provided with the distribution.         *
 *     * Neither the name EMStudio nor the                                            *
 *       names of its contributors may be used to endorse or promote products         *
 *       derived from this software without specific prior written permission.        *
 *                                                                                    *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND    *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED      *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE             *
 * DISCLAIMED. IN NO EVENT SHALL MICHAEL CARPENTER BE LIABLE FOR ANY                  *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES         *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;       *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND        *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT         *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS      *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                       *
 **************************************************************************************/

#ifndef MEMORYMETADATA_H
#define MEMORYMETADATA_H
#include <QString>
#include <QMap>
#include "table2dmetadata.h"
#include "table3dmetadata.h"
#include "readonlyramdata.h"
#include "lookupmetadata.h"
#include "configblock.h"
#include "menusetup.h"
class MemoryMetaData
{
public:
    virtual bool loadMetaDataFromFile(QString path) = 0;
    virtual bool parseMetaData(QString json) = 0;
    virtual const QMap<unsigned short, QString> errorMap() = 0;

    virtual bool has2DMetaData(unsigned short locationid) = 0;
    virtual const QList<Table2DMetaData> table2DMetaData() = 0;
    virtual const Table2DMetaData get2DMetaData(unsigned short locationid) = 0;

    virtual bool has3DMetaData(unsigned short locationid) = 0;
    virtual const QList<Table3DMetaData> table3DMetaData() = 0;
    virtual const Table3DMetaData get3DMetaData(unsigned short locationid) = 0;

    virtual bool hasRORMetaData(unsigned short locationid) = 0;
    virtual const ReadOnlyRamData getRORMetaData(unsigned short locationid) = 0;

    virtual bool hasLookupMetaData(unsigned short locationid) = 0;
    virtual const LookupMetaData getLookupMetaData(unsigned short locationid) = 0;

    virtual bool hasConfigMetaData(QString name) = 0;
    virtual const QMap<QString, QList<ConfigBlock>> configMetaData() = 0;
    virtual const QList<ConfigBlock> getConfigMetaData(QString name) = 0;
    virtual const MenuSetup menuMetaData() = 0;

    virtual const QString getErrorString(unsigned short code) = 0;
    virtual ~MemoryMetaData(){};
};

#endif // MEMORYMETADATA_H
