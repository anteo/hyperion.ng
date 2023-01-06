#pragma once

#include <QObject>
#include <QDataStream>
#include <QJsonObject>
#include "Connection.h"
#include "Settings.h"

namespace MagicLink
{
class Connection;

class Configuration: public QObject
{
Q_OBJECT
public:
	explicit Configuration(QObject* parent);
	explicit Configuration(const QJsonObject& properties, QObject* parent);

	bool isValid() const;
	QString printOut();
	bool read(Connection* conn);
	void write(Connection* conn);
	const Settings& current();
	const Settings& actual();
	int packetSize() const;
	bool isModified();
	void reset();

	void setUseCRC(UseCRC useCRC);
	void setLedType(LedType ledType);
	void setColorBits(ColorBits colorBits);
	void setColorCurve(ColorCurve colorCurve);
	void setLedsCount(int ledsCount);
	void setScalingThreshold(int scalingThreshold);
	void setSmoothingLevel(int smoothingLevel);

	UseCRC useCRC();
	LedType ledType();
	ColorBits colorBits();
	ColorCurve colorCurve();
	int ledsCount();
	int scalingThreshold() const;
	int scalingLevel();
	int smoothingLevel();
	int firmwareVersion();

	void fromJson(const QJsonObject& properties);
	QJsonObject toJson();

	explicit operator QJsonObject();

private:
	static int calculateAdvancedSettings(const Settings& conf);
	int calculateScalingLevel(const Settings& conf) const;
	void updateActualIfNeeded();

	int _scalingThreshold = 75;
	bool _valid = false;
	bool _needUpdateActual = false;
	Settings _current;
	Settings _actual;
	Settings _changes;
	void checkModified();
};

}
