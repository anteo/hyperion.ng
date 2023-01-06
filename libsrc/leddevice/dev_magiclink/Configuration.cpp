#include <QMetaEnum>
#include "Configuration.h"
#include "Utils.h"

namespace MagicLink
{
Configuration::Configuration(QObject* parent)
	: QObject(parent)
{}

bool Configuration::read(Connection* conn)
{
	QByteArray packet;
	if (!conn->receivePacket(packet))
		return false;

	quint8 leds, crc11, crc22, ledType, advancedSettings,
		firmwareVersion, smoothingLevel, sync;

	QDataStream data(packet);
	data >> firmwareVersion >> ledType >> leds >> advancedSettings
		 >> smoothingLevel >> sync >> crc11 >> crc22;

	quint8 crc1 = ledType ^ advancedSettings;
	quint8 crc2 = leds ^ sync;

	if (crc1 != crc11 || crc2 != crc22)
		return false;

	_current.smoothingLevel = smoothingLevel & 0x7F;
	_current.firmwareVersion = firmwareVersion;
	_current.advancedSettings = advancedSettings;
	_current.sync = sync;

	switch (advancedSettings & 0x0E)
	{
		case 0x02:
			_current.colorCurve = ColorCurve::CIE_CURVE;
			break;
		case 0x04:
			_current.colorCurve = ColorCurve::MIX_CURVE;
			break;
		case 0x08:
			_current.colorCurve = ColorCurve::S_CURVE;
			break;
		default:
			_current.colorCurve = ColorCurve::NORMAL_CURVE;
			break;
	}
	if (firmwareVersion > 8)
	{
		if (advancedSettings & 0x10) leds += 255;
		_current.scalingLevel = (ledType >> 1) & 7;
	}
	else
	{
		_current.scalingLevel = advancedSettings & 0x10 ? 2 : 1;
	}
	_current.ledsCount = leds;
	_current.ledType = ledType & 0x01 ? LedType::RGB : LedType::RGBW;
	_current.colorBits = advancedSettings & 0x20 ? ColorBits::RGB888 : ColorBits::RGB555;
	_current.useCRC = advancedSettings & 0x80 ? UseCRC::YES : UseCRC::NO;

	_valid = true;
	_needUpdateActual = true;
	updateActualIfNeeded();
	checkModified();
	return true;
}

bool Configuration::isValid() const
{
	return _valid;
}

QString Configuration::printOut()
{
	Settings conf1{current()};
	Settings conf2{actual()};

	QList<QStringList> rows;

	rows << (QStringList() << "PARAM" << "CURRENT" << "ACTUAL");
	rows << (QStringList() << "Firmware version"
						   << (conf1.firmwareVersion < 0 ? "-" : QString::number(conf1.firmwareVersion))
						   << (conf2.firmwareVersion < 0 ? "-" : QString::number(conf2.firmwareVersion)));
	rows << (QStringList() << "LEDs count"
						   << (conf1.ledsCount < 0 ? "-" : QString::number(conf1.ledsCount))
						   << (conf2.ledsCount < 0 ? "-" : QString::number(conf2.ledsCount)));
	rows << (QStringList() << "Scaling level"
						   << (conf1.scalingLevel < 0 ? "-" : QString::number(conf1.scalingLevel))
						   << (conf2.scalingLevel < 0 ? "-" : QString::number(conf2.scalingLevel)));
	rows << (QStringList() << "Advanced settings"
						   << (conf1.advancedSettings < 0 ? "-" : QString("0x%1").arg(conf1.advancedSettings, 2, 16, QLatin1Char('0')))
						   << (conf2.advancedSettings < 0 ? "-" : QString("0x%1").arg(conf2.advancedSettings, 2, 16, QLatin1Char('0'))));
	rows << (QStringList() << "Smoothing level"
						   << (conf1.smoothingLevel < 0 ? "-" : QString::number(conf1.smoothingLevel))
						   << (conf2.smoothingLevel < 0 ? "-" : QString::number(conf2.smoothingLevel)));
	rows << (QStringList() << "Sync"
						   << (conf1.sync < 0 ? "-" : QString::number(conf1.sync))
						   << (conf2.sync < 0 ? "-" : QString::number(conf2.sync)));
	rows << (QStringList() << "LED type"
						   << (conf1.ledType == LedType::UNSET ? "-" : QVariant::fromValue(conf1.ledType).toString())
						   << (conf2.ledType == LedType::UNSET ? "-" : QVariant::fromValue(conf2.ledType).toString()));
	rows << (QStringList() << "Color bits"
						   << (conf1.colorBits == ColorBits::UNSET ? "-" : QVariant::fromValue(conf1.colorBits).toString())
						   << (conf2.colorBits == ColorBits::UNSET ? "-" : QVariant::fromValue(conf2.colorBits).toString()));
	rows << (QStringList() << "Color curve"
						   << (conf1.colorCurve == ColorCurve::UNSET ? "-" : QVariant::fromValue(conf1.colorCurve).toString())
						   << (conf2.colorCurve == ColorCurve::UNSET ? "-" : QVariant::fromValue(conf2.colorCurve).toString()));
	rows << (QStringList() << "Use CRC"
						   << (conf1.useCRC == UseCRC::UNSET ? "-" : QVariant::fromValue(conf1.useCRC).toString())
						   << (conf2.useCRC == UseCRC::UNSET ? "-" : QVariant::fromValue(conf2.useCRC).toString()));

	return qAsciiTable(rows);
}

int Configuration::calculateAdvancedSettings(const Settings& conf)
{
	int settings = 0;
	if (conf.useCRC == UseCRC::YES) settings |= 0x80;
	if (conf.colorBits == ColorBits::RGB888) settings |= 0x20;
	if (conf.ledType == LedType::RGB) settings |= 0x01;
	if (conf.firmwareVersion > 8)
	{
		if (conf.ledsCount > 255) settings |= 0x10;
	}
	else
	{
		if (conf.scalingLevel > 1) settings |= 0x10;
	}
	switch (conf.colorCurve)
	{
		case ColorCurve::CIE_CURVE:
			settings |= 0x02;
			break;
		case ColorCurve::MIX_CURVE:
			settings |= 0x04;
			break;
		case ColorCurve::S_CURVE:
			settings |= 0x08;
			break;
		default:
			break;
	}
	return settings;
}

int Configuration::calculateScalingLevel(const Settings& conf) const
{
	if (conf.ledsCount <= _scalingThreshold)
		return 1;
	else if (conf.firmwareVersion <= 8)
		return 2;
	else
		return conf.ledsCount / _scalingThreshold + 1;
}

void Configuration::write(Connection* conn)
{
	QByteArray data;

	updateActualIfNeeded();

	if (_actual.ledType != _current.ledType)
	{
		data.clear();
		data.append(0x02);
		data.append(_actual.ledType == LedType::RGB ? 0x01 : 0x00);
		data.append('\0');
		conn->sendPacket(data, packetSize());
	}

	if (_actual.smoothingLevel != _current.smoothingLevel)
	{
		data.clear();
		data.append(0x11);
		data.append(_actual.smoothingLevel & 0x7F);
		conn->sendPacket(data, packetSize());
	}

	data.clear();
	if (_actual.ledsCount > 255)
	{
		data.append(0x40);
		data.append(static_cast<char>(_actual.ledsCount & 0xFF + 0x80));
	}
	else if (_actual.ledsCount > 127)
	{
		data.append(0x08);
		data.append(static_cast<char>(_actual.ledsCount & 0xFF + 0x80));
	}
	else
	{
		data.append(0x04);
		data.append(static_cast<char>(_actual.ledsCount));
	}
	data.append(static_cast<char>(_actual.advancedSettings));
	conn->sendPacket(data, packetSize());
}

const Settings& Configuration::current()
{
	return _current;
}

const Settings& Configuration::actual()
{
	updateActualIfNeeded();
	return _actual;
}

bool Configuration::isModified()
{
	updateActualIfNeeded();

	return _actual.useCRC != _current.useCRC ||
		_actual.ledType != _current.ledType ||
		_actual.colorBits != _current.colorBits ||
		_actual.colorCurve != _current.colorCurve ||
		_actual.ledsCount != _current.ledsCount ||
		_actual.smoothingLevel != _current.smoothingLevel ||
		_actual.scalingLevel != _current.scalingLevel ||
		_actual.advancedSettings != _current.advancedSettings;
}

int Configuration::packetSize() const
{
	int size;
	if (_current.scalingLevel <= 1 || _current.firmwareVersion > 8)
		size = _current.ledsCount;
	else
		size = (_current.ledsCount >> 1) + (_current.ledsCount & 1);
	if (_current.colorBits == ColorBits::RGB888)
		size *= 3;
	else
		size *= 2;
	size += 5;
	if (_current.firmwareVersion > 4)
		size += 4;
	return size;
}

void Configuration::checkModified()
{
	updateActualIfNeeded();

	if (_changes.useCRC != UseCRC::UNSET && _actual.useCRC == _current.useCRC)
		_changes.useCRC = UseCRC::UNSET;
	if (_changes.ledType != LedType::UNSET && _actual.ledType == _current.ledType)
		_changes.ledType = LedType::UNSET;
	if (_changes.colorBits != ColorBits::UNSET && _actual.colorBits == _current.colorBits)
		_changes.colorBits = ColorBits::UNSET;
	if (_changes.colorCurve != ColorCurve::UNSET && _actual.colorCurve == _current.colorCurve)
		_changes.colorCurve = ColorCurve::UNSET;
	if (_changes.ledsCount != -1 && _actual.ledsCount == _current.ledsCount)
		_changes.ledsCount = -1;
	if (_changes.smoothingLevel != -1 && _actual.smoothingLevel == _current.smoothingLevel)
		_changes.smoothingLevel = -1;
}

void Configuration::setUseCRC(MagicLink::UseCRC useCRC)
{
	_changes.useCRC = useCRC;
	_needUpdateActual = true;
}

void Configuration::setLedType(LedType ledType)
{
	_changes.ledType = ledType;
	_needUpdateActual = true;
}

void Configuration::setColorBits(ColorBits colorBits)
{
	_changes.colorBits = colorBits;
	_needUpdateActual = true;
}

void Configuration::setColorCurve(ColorCurve colorCurve)
{
	_changes.colorCurve = colorCurve;
	_needUpdateActual = true;
}

void Configuration::setLedsCount(int ledsCount)
{
	_changes.ledsCount = ledsCount;
	_needUpdateActual = true;
}

void Configuration::setScalingThreshold(int scalingThreshold)
{
	_scalingThreshold = scalingThreshold;
	_needUpdateActual = true;
}

void Configuration::setSmoothingLevel(int smoothingLevel)
{
	_changes.smoothingLevel = smoothingLevel;
	_needUpdateActual = true;
}

void Configuration::updateActualIfNeeded()
{
	if (!_needUpdateActual) return;
	_actual.useCRC = _changes.useCRC != UseCRC::UNSET ? _changes.useCRC : _current.useCRC;
	_actual.ledType = _changes.ledType != LedType::UNSET ? _changes.ledType : _current.ledType;
	_actual.colorBits = _changes.colorBits != ColorBits::UNSET ? _changes.colorBits : _current.colorBits;
	_actual.colorCurve = _changes.colorCurve != ColorCurve::UNSET ? _changes.colorCurve : _current.colorCurve;
	_actual.ledsCount = _changes.ledsCount != -1 ? _changes.ledsCount : _current.ledsCount;
	_actual.smoothingLevel = _changes.smoothingLevel != -1 ? _changes.smoothingLevel : _current.smoothingLevel;
	_actual.scalingLevel = calculateScalingLevel(_actual);
	_actual.advancedSettings = calculateAdvancedSettings(_actual);
	_actual.firmwareVersion = _current.firmwareVersion;
	_actual.sync = _current.sync;
	_needUpdateActual = false;
}

UseCRC Configuration::useCRC()
{
	return actual().useCRC;
}

LedType Configuration::ledType()
{
	return actual().ledType;
}

ColorBits Configuration::colorBits()
{
	return actual().colorBits;
}

ColorCurve Configuration::colorCurve()
{
	return actual().colorCurve;
}

int Configuration::ledsCount()
{
	return actual().ledsCount;
}

int Configuration::scalingThreshold() const
{
	return _scalingThreshold;
}

int Configuration::smoothingLevel()
{
	return actual().smoothingLevel;
}

Configuration::Configuration(const QJsonObject& properties, QObject* parent)
	: Configuration(parent)
{
	fromJson(properties);
}

Configuration::operator QJsonObject()
{
	return toJson();
}

int Configuration::firmwareVersion()
{
	return actual().firmwareVersion;
}

int Configuration::scalingLevel()
{
	return actual().scalingLevel;
}

QJsonObject Configuration::toJson()
{
	QJsonObject properties;
	properties.insert("useCRC", useCRC() == MagicLink::UseCRC::YES);
	properties.insert("ledType", QVariant::fromValue(ledType()).toString());
	properties.insert("colorBits", QVariant::fromValue(colorBits()).toString());
	properties.insert("colorCurve", QVariant::fromValue(colorCurve()).toString());
	properties.insert("currentLedCount", ledsCount());
	properties.insert("firmwareVersion", firmwareVersion());
	properties.insert("smoothingLevel", smoothingLevel());
	properties.insert("scalingThreshold", scalingThreshold());
	properties.insert("scalingLevel", scalingLevel());
	return properties;
}

void Configuration::fromJson(const QJsonObject& properties)
{
	if (properties.contains("ledType") && properties["ledType"].isString())
	{
		auto&& metaEnum = QMetaEnum::fromType<MagicLink::LedType>();
		auto ledType = properties["ledType"].toString().toStdString();
		setLedType(static_cast<MagicLink::LedType>(metaEnum.keyToValue(ledType.c_str())));
	}
	if (properties.contains("colorBits") && properties["colorBits"].isString())
	{
		auto&& metaEnum = QMetaEnum::fromType<MagicLink::ColorBits>();
		auto colorBits = properties["colorBits"].toString().toStdString();
		setColorBits(static_cast<MagicLink::ColorBits>(metaEnum.keyToValue(colorBits.c_str())));
	}
	if (properties.contains("colorCurve") && properties["colorCurve"].isString())
	{
		auto&& metaEnum = QMetaEnum::fromType<MagicLink::ColorCurve>();
		auto colorCurve = properties["colorCurve"].toString().toStdString();
		setColorCurve(static_cast<MagicLink::ColorCurve>(metaEnum.keyToValue(colorCurve.c_str())));
	}
	if (properties.contains("useCRC") && properties["useCRC"].isBool())
		setUseCRC(properties["useCRC"].toBool() ? MagicLink::UseCRC::YES : MagicLink::UseCRC::NO);
	if (properties.contains("currentLedCount") && properties["currentLedCount"].isDouble())
		setLedsCount(properties["currentLedCount"].toInt());
	if (properties.contains("smoothingLevel") && properties["smoothingLevel"].isDouble())
		setSmoothingLevel(properties["smoothingLevel"].toInt());
	if (properties.contains("scalingThreshold") && properties["scalingThreshold"].isDouble())
		setScalingThreshold(properties["scalingThreshold"].toInt());
}

void Configuration::reset()
{
	_changes = Settings();
	_needUpdateActual = true;
}

}
