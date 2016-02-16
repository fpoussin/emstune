/************************************************************************************
 * EMStudio - Open Source ECU tuning software                                       *
 * Copyright (C) 2013  Michael Carpenter (malcom2073@gmail.com)                     *
 *                                                                                  *
 * This file is a part of EMStudio                                                  *
 *                                                                                  *
 * EMStudio is free software; you can redistribute it and/or                        *
 * modify it under the terms of the GNU Lesser General Public                       *
 * License as published by the Free Software Foundation, version                    *
 * 2.1 of the License.                                                              *
 *                                                                                  *
 * EMStudio is distributed in the hope that it will be useful,                      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 * Lesser General Public License for more details.                                  *
 *                                                                                  *
 * You should have received a copy of the GNU Lesser General Public                 *
 * License along with this program; if not, write to the Free Software              *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA   *
 ************************************************************************************/

#include "fetable2ddata.h"
#include "formattype.h"

#include <QDebug>
FETable2DData::FETable2DData(bool is32bit) : Table2DData()
{
	m_is32Bit = is32bit;
	m_writesEnabled = true;
	m_acccessMutex = new QMutex();
}
void FETable2DData::writeWholeLocation(bool ram)
{
	if (ram)
	{
		emit saveSingleDataToRam(m_locationId,0,data().size(),data());
	}
	else
	{
		emit saveSingleDataToFlash(m_locationId,0,data().size(),data());
	}
}
void FETable2DData::setWritesEnabled(bool enabled)
{
	m_writesEnabled = enabled;
}
void FETable2DData::reCalcAxisData()
{
	if (m_is32Bit)
	{
		m_minActualXAxis = calcAxis(UINT_MAX,x_metaData);
		m_minActualYAxis = calcAxis(UINT_MAX,y_metaData);
		m_maxActualXAxis = calcAxis(0,x_metaData);
		m_maxActualYAxis = calcAxis(0,y_metaData);

	}
	else
	{
		m_minActualXAxis = calcAxis(65535,x_metaData);
		m_minActualYAxis = calcAxis(65535,y_metaData);
		m_maxActualXAxis = calcAxis(0,x_metaData);
		m_maxActualYAxis = calcAxis(0,y_metaData);
	}
	for (int i=0;i<m_axis.size();i++)
	{
		if (m_axis[i] > m_maxActualXAxis)
		{
			m_maxActualXAxis = m_axis[i];
		}
		if (m_axis[i] < m_minActualXAxis)
		{
			m_minActualXAxis = m_axis[i];
		}

	}
	for (int i=0;i<m_values.size();i++)
	{

		if (m_values[i] > m_maxActualYAxis)
		{
			m_maxActualYAxis = m_values[i];
		}
		if (m_values[i] < m_minActualYAxis)
		{
			m_minActualYAxis = m_values[i];
		}
	}
}

void FETable2DData::setMetaData(TableMeta metadata,FieldMeta xMeta,FieldMeta yMeta)
{
	m_tableMeta = metadata;
	x_metaData = xMeta;
	y_metaData = yMeta;
}

void FETable2DData::setData(unsigned short locationid, bool isflashonly,QByteArray payload)
{

	if (locationid == 0x10E)
	{
		int stopper = 1;
	}
	//0x10E
	m_acccessMutex->lock();
	m_dataSize = payload.size();
	//m_isSignedData = signedData;
	m_isFlashOnly = isflashonly;

	m_locationId = locationid;
	m_axis.clear();
	m_values.clear();





	if (x_metaData.size == 32)
	{
		m_maxCalcedXAxis = calcAxis(INT_MAX,x_metaData);
	}
	else
	{
		m_maxCalcedXAxis = calcAxis(65535,x_metaData);
	}
	if (y_metaData.size == 32)
	{
		m_maxCalcedYAxis = calcAxis(INT_MAX,y_metaData);
	}
	else
	{
		m_maxCalcedYAxis = calcAxis(65535,y_metaData);
	}

	if (x_metaData.isSigned)
	{
		if (x_metaData.size == 32)
		{
			m_minCalcedXAxis = calcAxis(INT_MIN,x_metaData);
		}
		else
		{
			m_minCalcedXAxis = calcAxis(-65535,x_metaData);
		}
	}
	else
	{
		m_minCalcedXAxis = calcAxis(0,x_metaData);
	}
	if (y_metaData.isSigned)
	{
		if (y_metaData.size == 32)
		{
			m_minCalcedYAxis = calcAxis(INT_MIN,y_metaData);
		}
		else
		{
			m_minCalcedYAxis = calcAxis(-65535,y_metaData);
		}
	}
	else
	{
		m_minCalcedYAxis = calcAxis(0,y_metaData);
	}
	//Reverse the min and max, so we can figure them out based on real data
	m_minActualXAxis = calcAxis(INT_MAX,x_metaData);
	m_minActualYAxis = calcAxis(INT_MAX,y_metaData);
	m_maxActualXAxis = calcAxis(0,x_metaData);
	m_maxActualYAxis = calcAxis(0,y_metaData);
	qDebug() << "32bit 2d table!" << QString::number(locationid,16).toUpper();
	int valuecount = 0;
	if (m_tableMeta.formatId == TABLE_2D_STRUCTURED)
	{
		int adder = 4;
		if (y_metaData.size == 32)
		{
			adder = 6;
		}
		for (int i=0;i<payload.size();i+=adder)
		{
			double xdouble = 0;
			double ydouble = 0;

			//Iterate through all the axisvalues
			unsigned short x = (((unsigned char)payload[i]) << 8) + ((unsigned char)payload[i+1]);
			if (x_metaData.isSigned)
			{
				xdouble = calcAxis((short)x,x_metaData);
			}
			else
			{
				xdouble = calcAxis(x,x_metaData);
			}
			//unsigned int y = (((unsigned char)payload[(payload.size()/3)+ valuecount]) << 24) + (((unsigned char)payload[(payload.size()/3)+ valuecount+1]) << 16) + (((unsigned char)payload[(payload.size()/3)+ valuecount+2]) << 8) + ((unsigned char)payload[(payload.size()/3) + valuecount+3]);
			if (y_metaData.size == 32)
			{
				unsigned int y = (((unsigned char)payload[i+2]) << 24) + (((unsigned char)payload[i+3]) << 16) << (((unsigned char)payload[i+4]) << 8) << (((unsigned char)payload[i+5]));
				if (y_metaData.isSigned)
				{
					ydouble = calcAxis((short)y,y_metaData);
				}
				else
				{
					ydouble = calcAxis(y,y_metaData);
				}
			}
			else
			{
				unsigned short y = (((unsigned char)payload[i+2]) << 8) + ((unsigned char)payload[i+3]);
				if (y_metaData.isSigned)
				{
					ydouble = calcAxis((short)y,y_metaData);
				}
				else
				{
					ydouble = calcAxis(y,y_metaData);
				}
			}
			if (xdouble > m_maxActualXAxis)
			{
				m_maxActualXAxis = xdouble;
			}
			if (xdouble < m_minActualXAxis)
			{
				m_minActualXAxis = xdouble;
			}

			if (ydouble > m_maxActualYAxis)
			{
				m_maxActualYAxis = ydouble;
			}
			if (ydouble < m_minActualYAxis)
			{
				m_minActualYAxis = ydouble;
			}

			m_axis.append(xdouble);
			m_values.append(ydouble);
		}
	}
	else if (m_tableMeta.formatId == TABLE_2D_LEGACY)
	{
		for (int i=0;i<payload.size()/3;i+=2)
		{
			//Iterate through all the axisvalues
			unsigned short x = (((unsigned char)payload[i]) << 8) + ((unsigned char)payload[i+1]);
			unsigned int y = (((unsigned char)payload[(payload.size()/3)+ valuecount]) << 24) + (((unsigned char)payload[(payload.size()/3)+ valuecount+1]) << 16) + (((unsigned char)payload[(payload.size()/3)+ valuecount+2]) << 8) + ((unsigned char)payload[(payload.size()/3) + valuecount+3]);
			//unsigned int y = (((unsigned char)payload[i+2]) << 24) + (((unsigned char)payload[i+3]) << 16) << (((unsigned char)payload[i+4]) << 8) << (((unsigned char)payload[i+5]));
			valuecount+=4;
			double xdouble = 0;
			double ydouble = 0;
			if (m_isSignedData)
			{
				xdouble = calcAxis((short)x,x_metaData);
				ydouble = calcAxis((short)y,y_metaData);
			}
			else
			{
				xdouble = calcAxis(x,x_metaData);
				ydouble = calcAxis(y,y_metaData);
			}
			if (xdouble > m_maxActualXAxis)
			{
				m_maxActualXAxis = xdouble;
			}
			if (xdouble < m_minActualXAxis)
			{
				m_minActualXAxis = xdouble;
			}

			if (ydouble > m_maxActualYAxis)
			{
				m_maxActualYAxis = ydouble;
			}
			if (ydouble < m_minActualYAxis)
			{
				m_minActualYAxis = ydouble;
			}

			m_axis.append(xdouble);
			m_values.append(ydouble);
		}
	}


	m_acccessMutex->unlock();
	emit update();
}
double FETable2DData::maxActualXAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_maxActualXAxis;
}

double FETable2DData::maxActualYAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_maxActualYAxis;
}

double FETable2DData::minActualXAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_minActualXAxis;
}

double FETable2DData::minActualYAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_minActualYAxis;
}
double FETable2DData::maxCalcedXAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_maxCalcedXAxis;
}

double FETable2DData::maxCalcedYAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_maxCalcedYAxis;
}

double FETable2DData::minCalcedXAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_minCalcedXAxis;
}

double FETable2DData::minCalcedYAxis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_minCalcedYAxis;
}

QList<double> FETable2DData::axis()
{
	QMutexLocker locker(m_acccessMutex);
	return m_axis;
}

QList<double> FETable2DData::values()
{
	QMutexLocker locker(m_acccessMutex);
	return m_values;
}
int FETable2DData::columns()
{
	QMutexLocker locker(m_acccessMutex);
	return m_axis.size();
}

int FETable2DData::rows()
{
	QMutexLocker locker(m_acccessMutex);
	return 2;
}

void FETable2DData::setCell(int row, int column,double newval)
{
	QMutexLocker locker(m_acccessMutex);
	//New value has been accepted. Let's write it.
	//offset = column + (row * 32), size == 2
	//QLOG_DEBUG() << "Update:" << row << column << newval;
	quint64 val = 0;
	QByteArray data;
	if (row == 0)
	{
		val = backConvertAxis(newval,x_metaData);
		m_axis.replace(column,newval);
		if (x_metaData.isSigned)
		{
			if (x_metaData.size == 32)
			{
				data.append((char)(((qint32)val >> 24) & 0xFF));
				data.append((char)(((qint32)val >> 16) & 0xFF));
				data.append((char)(((qint32)val >> 8) & 0xFF));
				data.append((char)(((qint32)val) & 0xFF));
			}
			else
			{
				data.append((char)(((short)val >> 8) & 0xFF));
				data.append((char)((short)val & 0xFF));
			}
		}
		else
		{
			if (x_metaData.size == 32)
			{
				data.append((char)(((quint32)val >> 24) & 0xFF));
				data.append((char)(((quint32)val >> 16) & 0xFF));
				data.append((char)(((quint32)val >> 8) & 0xFF));
				data.append((char)(((quint32)val) & 0xFF));
			}
			else
			{
				data.append((char)((((unsigned short)val) >> 8) & 0xFF));
				data.append((char)(((unsigned short)val) & 0xFF));
			}

		}
	}
	else if (row == 1)
	{

		val = backConvertAxis(newval,y_metaData);
		m_values.replace(column,newval);
		if (y_metaData.isSigned)
		{
			if (y_metaData.size == 32)
			{
				data.append((char)(((qint32)val >> 24) & 0xFF));
				data.append((char)(((qint32)val >> 16) & 0xFF));
				data.append((char)(((qint32)val >> 8) & 0xFF));
				data.append((char)(((qint32)val) & 0xFF));
			}
			else
			{
				data.append((char)(((short)val >> 8) & 0xFF));
				data.append((char)((short)val & 0xFF));
			}
		}
		else
		{
			if (y_metaData.size == 32)
			{
				data.append((char)(((quint32)val >> 24) & 0xFF));
				data.append((char)(((quint32)val >> 16) & 0xFF));
				data.append((char)(((quint32)val >> 8) & 0xFF));
				data.append((char)(((quint32)val) & 0xFF));
			}
			else
			{
				data.append((char)((((unsigned short)val) >> 8) & 0xFF));
				data.append((char)(((unsigned short)val) & 0xFF));
			}

		}
	}

	reCalcAxisData();
	if (!m_isFlashOnly)
	{
		if (m_writesEnabled)
		{
			if (m_tableMeta.formatId == TABLE_2D_STRUCTURED)
			{
				if (x_metaData.size == 32)
				{
					if (y_metaData.size == 32)
					{
						emit saveSingleDataToRam(m_locationId,(column*8) + (row*4),data.size(),data);
					}
					else
					{
						emit saveSingleDataToRam(m_locationId,(column*6) + (row*2),data.size(),data);
					}
				}
				else if (y_metaData.size == 32)
				{
					emit saveSingleDataToRam(m_locationId,(column*6) + (row*4),data.size(),data);
				}
				else
				{
					emit saveSingleDataToRam(m_locationId,((column)*4) + ((row)*2),data.size(),data);
				}
			}
			else if (m_tableMeta.formatId == TABLE_2D_LEGACY)
			{
				if (y_metaData.size == 32)
				{
					emit saveSingleDataToRam(m_locationId,(column*4) + (row * m_tableMeta.size / 3.0),4,data);
				}
				else
				{
					emit saveSingleDataToRam(m_locationId,(column*4) + (row * m_tableMeta.size / 3.0),4,data);
				}
			}
			else
			{
				//Error
				int stopper = 1;
			}

			/*if (m_metaData.valid)
			{
				if (m_is32Bit)
				{
					emit saveSingleDataToRam(m_locationId,(column*4) + (row * m_tableMeta.size / 3.0),4,data);
				}
				else
				{
				    //emit saveSingleDataToRam(m_locationId,(column*2)+(row * (m_metaData.size / 2.0)),2,data);
					emit saveSingleDataToRam(m_locationId,(column*2+(row*2)),2,data);

				}
			}
			else
			{
				if (m_is32Bit)
				{
					emit saveSingleDataToRam(m_locationId,(column*4) + (row * m_dataSize / 3.0),4,data);
				}
				else
				{
					//emit saveSingleDataToRam(m_locationId,(column*2)+(row * (m_dataSize / 2.0)),2,data);
					emit saveSingleDataToRam(m_locationId,(column*2+(row*2)),2,data);
				}
			}*/
		}
	}
}

QByteArray FETable2DData::data()
{
	QMutexLocker locker(m_acccessMutex);
	QByteArray data;
	if (m_tableMeta.formatId == TABLE_2D_LEGACY)
	{
		for (int i=0;i<m_axis.size();i++)
		{
			unsigned short axis = backConvertAxis(m_axis[i],x_metaData);
			data.append((char)((axis >> 8) & 0xFF));
			data.append((char)(axis & 0xFF));
		}
		for (int i=0;i<m_values.size();i++)
		{
			if (y_metaData.size == 32)
			{
				quint64 val = backConvertAxis(m_values[i],y_metaData);
				data.append((char)((val >> 24) & 0xFF));
				data.append((char)((val >> 16) & 0xFF));
				data.append((char)((val >> 8) & 0xFF));
				data.append((char)(val & 0xFF));
			}
			else
			{
				unsigned short val = backConvertAxis(m_values[i],y_metaData);
				data.append((char)((val >> 8) & 0xFF));
				data.append((char)(val & 0xFF));
			}
		}
	}
	else if (m_tableMeta.formatId == TABLE_2D_STRUCTURED)
	{
		for (int i=0;i<m_axis.size();i++)
		{
			unsigned short axis = backConvertAxis(m_axis[i],x_metaData);
			data.append((char)((axis >> 8) & 0xFF));
			data.append((char)(axis & 0xFF));
			if (y_metaData.size == 32)
			{
				quint64 val = backConvertAxis(m_values[i],y_metaData);
				data.append((char)((val >> 24) & 0xFF));
				data.append((char)((val >> 16) & 0xFF));
				data.append((char)((val >> 8) & 0xFF));
				data.append((char)(val & 0xFF));
			}
			else
			{
				unsigned short val = backConvertAxis(m_values[i],y_metaData);
				data.append((char)((val >> 8) & 0xFF));
				data.append((char)(val & 0xFF));
			}
		}
	}
	return data;
}
double FETable2DData::calcAxis(qint64 val,FieldMeta metadata)
{
	if (m_tableMeta.valid)
	{
		double newval = (val * metadata.multiplier) + metadata.adder;
		return newval;
	}
	return (double)val;
}
quint64 FETable2DData::backConvertAxis(double val,FieldMeta metadata)
{
	if (m_tableMeta.valid)
	{
		double newval = (val - metadata.adder) / metadata.multiplier;
		if (m_is32Bit)
		{
			return (quint32)newval;
		}
		else
		{
			return (quint16)newval;
		}
	}
	return (quint64)val;
}
void FETable2DData::updateFromFlash()
{
	//emit requestBlockFromFlash(m_locationId,0,0);
	emit requestRamUpdateFromFlash(m_locationId);
}

void FETable2DData::updateFromRam()
{
	//emit requestRamUpdateFromFlash(m_locationId);
	emit requestBlockFromRam(m_locationId,0,0);
}
void FETable2DData::saveRamToFlash()
{
	emit requestFlashUpdateFromRam(m_locationId);
}
