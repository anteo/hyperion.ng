#ifndef LEDEVICEMAGICLINK_H
#define LEDEVICEMAGICLINK_H

#include <QEventLoop>
#include <leddevice/LedDevice.h>
#include "Configuration.h"
#include "DiscoveryAgent.h"

class LedDeviceMagicLink: public LedDevice
{
public:

	explicit LedDeviceMagicLink(const QJsonObject& deviceConfig);
	~LedDeviceMagicLink() override;

	static LedDevice* construct(const QJsonObject& deviceConfig);

	QJsonObject discover(const QJsonObject& params) override;
	QJsonObject getProperties(const QJsonObject& params) override;

protected:

	bool init(const QJsonObject& deviceConfig) override;

	int open() override;

	int close() override;

	int write(const std::vector<ColorRgb>& ledValues) override;

private:

	QString _address;
	MagicLink::Connection* _conn{};
	MagicLink::DiscoveryAgent* _discoveryAgent{};
	QEventLoop* _loop;

	MagicLink::DiscoveryAgent* discoveryAgent();
	MagicLink::Connection* connection();
};

#endif // LEDEVICEMAGICLINK_H
