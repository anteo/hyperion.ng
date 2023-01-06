#include <QDBusConnection>
#include <iostream>
#include "PairingAgent.h"

namespace MagicLink
{
const QString agentPath = "/pairing/agent";

const QVariant agentObject = QVariant::fromValue(QDBusObjectPath(agentPath));

QString PairingAgent::RequestPinCode(const QDBusObjectPath& device) const
{
	Debug(_log, "PIN has been requested.");
	return _pinCode;
}

PairingAgent::PairingAgent(QObject* parent)
	: QObject(parent)
	  , _started(false)
	  , _adaptor(new PairingAgentAdaptor(this))
	  , _agentManager(new QDBusInterface("org.bluez", "/org/bluez", "org.bluez.AgentManager1",
										 QDBusConnection::systemBus(), this))
	  , _log(Logger::getInstance("MAGICLINK"))
{
}

bool PairingAgent::start(const QString& pinCode)
{
	QDBusConnection systemBus(QDBusConnection::systemBus());

	if (_started) return true;

	if (systemBus.registerObject(agentPath, this))
	{
		Debug(_log, "Pairing agent has been registered at %s", QSTRING_CSTR(agentPath));
	}
	else
	{
		Warning(_log, "Failed to register pairing agent: %s", QSTRING_CSTR(systemBus.lastError().message()));
		return false;
	}

	if (!_agentManager->isValid())
	{
		Warning(_log, "Failed to get agent manager: %s", QSTRING_CSTR(_agentManager->lastError().message()));
		return false;
	}

	if (!call("RegisterAgent", agentObject, "DisplayOnly")) return false;
	if (!call("RequestDefaultAgent", agentObject)) return false;

	Debug(_log, "Default agent has been successfully requested.");
	_pinCode = pinCode;
	_started = true;

	return true;
}

bool PairingAgent::stop()
{
	if (_started)
	{
		_started = false;
		call("UnregisterAgent", agentObject);
		QDBusConnection::systemBus().unregisterObject(agentPath, QDBusConnection::UnregisterTree);
		return true;
	}
	else
	{
		return false;
	}
}

template<typename... Args>
bool PairingAgent::call(const QString& method, Args&& ... args)
{
	QDBusReply<void> reply = _agentManager->call(method, args...);
	if (!reply.isValid())
	{
		Warning(_log, "Failed to %s: %s", QSTRING_CSTR(method), QSTRING_CSTR(reply.error().message()));
		return false;
	}
	return true;
}

PairingAgent::~PairingAgent()
{
	stop();
}

}
