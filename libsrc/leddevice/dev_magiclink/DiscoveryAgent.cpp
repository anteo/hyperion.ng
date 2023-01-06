#include <QDBusInterface>
#include <QDBusReply>
#include "DiscoveryAgent.h"

namespace MagicLink
{

const QString magicLinkName = "MagicLink";

const QList<QString> knownMacAddresses = {
	"00:18:E4",
	"20:17",
	"00:19",
	"98:D3",
	"20:20",
	"00:20",
	"00:21",
	"20:19",
	"98:DA"
};

DiscoveryAgent::DiscoveryAgent(QObject* parent)
	: QObject(parent)
	  , _timeoutTimer(new QTimer(this))
	  , _restartTimer(new QTimer(this))
	  , _log(Logger::getInstance("MAGICLINK"))
{
	_timeoutTimer->setSingleShot(true);
	_restartTimer->setSingleShot(true);

	_timeoutTimer->callOnTimeout(this, &DiscoveryAgent::handleFinished);
	_restartTimer->callOnTimeout(this, &DiscoveryAgent::nextRound);

	_acceptedDevices = new QList<QBluetoothDeviceInfo>;
	_devicesForCleanup = new QList<QBluetoothAddress>;
}

void DiscoveryAgent::start()
{
	if (_started)
	{
		Info(_log, "Discovery agent is already started.");
		return;
	}
	_errorsCount = _roundsCount = 0;
	_acceptedDevices->clear();
	_devicesForCleanup->clear();
	nextRound();
}

void DiscoveryAgent::stop()
{
	if (!_started) return;
	_discoveryAgent->stop();
	_timeoutTimer->stop();
	_restartTimer->stop();
	_started = false;
}

QBluetoothDeviceDiscoveryAgent* DiscoveryAgent::discoveryAgent() const
{
	return _discoveryAgent;
}

int DiscoveryAgent::roundsCount() const
{
	return _roundsCount;
}

int DiscoveryAgent::errorsCount() const
{
	return _errorsCount;
}

bool DiscoveryAgent::isStarted() const
{
	return _started;
}

QList<QBluetoothDeviceInfo>* DiscoveryAgent::acceptedDevices() const
{
	return _acceptedDevices;
}

void DiscoveryAgent::handleDeviceDiscovered(const QBluetoothDeviceInfo& info)
{
	bool accepted = checkDeviceAccepted(info);
	Info(_log, "Device discovered: %s, accepted: %s", QSTRING_CSTR(deviceDescription(info)), accepted ? "YES" : "NO");
}

void DiscoveryAgent::handleDeviceUpdated(const QBluetoothDeviceInfo& info)
{
	bool accepted = checkDeviceAccepted(info);
	Info(_log, "Device updated: %s, accepted: %s", QSTRING_CSTR(deviceDescription(info)), accepted ? "YES" : "NO");
}

bool DiscoveryAgent::isAccepted(const QBluetoothDeviceInfo& info)
{
	const QBluetoothAddress& address = info.address();
	bool isMatched = info.name().startsWith(magicLinkName) &&
		std::any_of(knownMacAddresses.begin(), knownMacAddresses.end(),
					[&address](const QString& addr)
					{ return address.toString().contains(addr); });
	bool isCached = info.rssi() == 0;
	if (isMatched)
	{
		if (isCached && !_devicesForCleanup->contains(address))
			_devicesForCleanup->append(address);
		if (!isCached && _devicesForCleanup->contains(address))
			_devicesForCleanup->removeAll(address);
	}
	return isMatched && !isCached;
}

bool DiscoveryAgent::checkDeviceAccepted(const QBluetoothDeviceInfo& info)
{
	bool accepted = isAccepted(info);
	if (accepted && !_acceptedDevices->contains(info))
	{
		_acceptedDevices->append(info);
		emit deviceDiscovered(info);
	}
	return accepted;
}

void DiscoveryAgent::handleError()
{
	_errorsCount++;
	Warning(_log, "Discovery error: %s", QSTRING_CSTR(_discoveryAgent->errorString()));
	emit error(_discoveryAgent->errorString());
	if (_maxErrors > 0 && _errorsCount >= _maxErrors)
	{
		stop();
		Error(_log, "Errors count exceeded the limit (%d), giving up...", _maxErrors);
		emit finished();
	}
}

void DiscoveryAgent::handleFinished()
{
	Info(_log, "Discovery round %d finished.", _roundsCount);
	for (const auto& info: _discoveryAgent->discoveredDevices())
	{
		handleDeviceUpdated(info);
	}
	stop();
	if (_acceptedDevices->isEmpty())
	{
		if (_maxRounds > 0 && _roundsCount >= _maxRounds)
		{
			Warning(_log, "Rounds count exceeded the limit (%d), giving up...", _maxRounds);
			emit finished();
		}
		else
		{
			Info(_log, "No suitable devices found, retrying discovery...");
			_restartTimer->start(5000);
		}
	}
	else
	{
		Info(_log, "%d devices found", _acceptedDevices->length());
		emit finished();
	}
}

void DiscoveryAgent::nextRound()
{
	Info(_log, "Starting discovery agent (round %d)...", ++_roundsCount);

	_localDevice = new QBluetoothLocalDevice(this);
	if (!_localDevice->isValid())
	{
		_restartTimer->start(10000);
		Error(_log, "No bluetooth device is available.");
		return;
	}
	else
	{
		_localDevice->powerOn();
		Info(_log, "Local device address: %s", QSTRING_CSTR(_localDevice->address().toString()));
	}

	_timeoutTimer->start(_timeoutSec * 1000);

	delete _discoveryAgent;
	_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(_localDevice->address(), this);
	connect(_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
			this, &DiscoveryAgent::handleDeviceDiscovered);
	connect(_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated,
			this, &DiscoveryAgent::handleDeviceUpdated);
	connect(_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
			this, &DiscoveryAgent::handleFinished);
	connect(_discoveryAgent, static_cast<void (QBluetoothDeviceDiscoveryAgent::*)(
				QBluetoothDeviceDiscoveryAgent::Error)>(&QBluetoothDeviceDiscoveryAgent::error),
			this, &DiscoveryAgent::handleError);
	_discoveryAgent->start();
	_started = true;
}

QString DiscoveryAgent::deviceDescription(const QBluetoothDeviceInfo& info)
{
	return QStringLiteral("%1 (%2, RSSI: %3)").arg(info.name(), info.address().toString()).arg(info.rssi());
}

void DiscoveryAgent::setMaxRounds(int maxRounds)
{
	_maxRounds = maxRounds;
}

void DiscoveryAgent::setMaxErrors(int maxErrors)
{
	_maxErrors = maxErrors;
}

void DiscoveryAgent::setTimeoutSec(int timeoutSec)
{
	_timeoutSec = timeoutSec;
}

bool DiscoveryAgent::removeDevice(const QBluetoothAddress& address)
{
	Info(_log, "Remove device: %s", QSTRING_CSTR(address.toString()));
	QDBusInterface adapter("org.bluez", "/org/bluez/hci0", "org.bluez.Adapter1",
						   QDBusConnection::systemBus());
	if (!adapter.isValid())
	{
		Error(_log, "Failed to get adapter interface: %s", QSTRING_CSTR(adapter.lastError().message()));
		return false;
	}

	QString objectPath(QStringLiteral("/org/bluez/hci0/dev_%1").arg(address.toString().replace(":", "_")));
	QDBusReply<void> reply = adapter.call("RemoveDevice", QVariant::fromValue(QDBusObjectPath(objectPath)));
	if (!reply.isValid())
	{
		Error(_log, "Failed to remove device %s: %s", QSTRING_CSTR(objectPath), QSTRING_CSTR(reply.error().message()));
		return false;
	}
	return true;
}

DiscoveryAgent::~DiscoveryAgent()
{
	stop();
}

void DiscoveryAgent::cleanupCachedDevices()
{
	for (const auto& address : *_devicesForCleanup)
		removeDevice(address);
}

}
