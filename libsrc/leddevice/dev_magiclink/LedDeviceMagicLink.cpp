#include <QEventLoop>
#include "LedDeviceMagicLink.h"
#include "DiscoveryAgent.h"
#include "Connection.h"

namespace
{
const char CONFIG_HOST[] = "host";
}

LedDeviceMagicLink::LedDeviceMagicLink(const QJsonObject& deviceConfig)
	: LedDevice(deviceConfig)
	  , _loop(new QEventLoop(this))
{
}

LedDeviceMagicLink::~LedDeviceMagicLink()
{
	delete _discoveryAgent;
	_discoveryAgent = nullptr;
	delete _conn;
	_conn = nullptr;
}

LedDevice* LedDeviceMagicLink::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceMagicLink(deviceConfig);
}

bool LedDeviceMagicLink::init(const QJsonObject& deviceConfig)
{
	bool initOK = LedDevice::init(deviceConfig);

	_address = deviceConfig[CONFIG_HOST].toString();
	connection()->setDeviceAddress(_address);
	connection()->configuration()->fromJson(deviceConfig);

	Debug(_log, "\n%s", QSTRING_CSTR(connection()->configuration()->printOut()));

	return initOK;
}

int LedDeviceMagicLink::open()
{
	int retval = -1;
	_isDeviceReady = false;

	connect(connection(), &MagicLink::Connection::socketError, this, &LedDeviceMagicLink::setInError);
	connection()->initiate();
	_loop->exec();

	if (connection()->isReady())
	{
		_isDeviceReady = true;
		retval = 0;
	}
	return retval;
}

int LedDeviceMagicLink::close()
{
	LedDevice::close();
	int retval = 0;
	connection()->finish();
	return retval;
}

int LedDeviceMagicLink::write(const std::vector<ColorRgb>& ledValues)
{
	connection()->sendColors(ledValues);
	return 0;
}

QJsonObject LedDeviceMagicLink::discover(const QJsonObject& params)
{
	discoveryAgent()->start();
	_loop->exec();

	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType);
	QJsonArray deviceList;

	for (const auto& device: *_discoveryAgent->acceptedDevices())
	{
		QJsonObject obj;
		obj.insert("address", device.address().toString());
		obj.insert("name", device.name());
		obj.insert("rssi", device.rssi());
		deviceList << obj;
	}

	devicesDiscovered.insert("devices", deviceList);
	return devicesDiscovered;
}

QJsonObject LedDeviceMagicLink::getProperties(const QJsonObject& params)
{
	QJsonObject properties;
	QJsonObject deviceProperties;

	_address = params[CONFIG_HOST].toString("");
	if (!_address.isEmpty())
	{
		connection()->setDeviceAddress(_address);
		connection()->initiate();
		_loop->exec();

		if (connection()->isReady())
			deviceProperties = connection()->configuration()->toJson();

		properties.insert("properties", deviceProperties);
	}

	return properties;
}

MagicLink::DiscoveryAgent* LedDeviceMagicLink::discoveryAgent()
{
	if (_discoveryAgent) return _discoveryAgent;

	_discoveryAgent = new MagicLink::DiscoveryAgent();
	_discoveryAgent->setTimeoutSec(10);
	_discoveryAgent->setMaxErrors(1);
	_discoveryAgent->setMaxRounds(1);
	connect(_discoveryAgent, &MagicLink::DiscoveryAgent::finished, _loop, &QEventLoop::quit);
	return _discoveryAgent;
}

MagicLink::Connection* LedDeviceMagicLink::connection()
{
	if (_conn) return _conn;

	_conn = new MagicLink::Connection();
	_conn->setMaxErrors(1);
	connect(_conn, &MagicLink::Connection::ready, _loop, &QEventLoop::quit);
	connect(_conn, &MagicLink::Connection::socketError, _loop, &QEventLoop::quit);
	connect(_conn, &MagicLink::Connection::connectionError, _loop, &QEventLoop::quit);
	return _conn;
}
