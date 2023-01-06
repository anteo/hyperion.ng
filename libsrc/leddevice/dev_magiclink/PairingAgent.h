#pragma once

#include <QObject>
#include "PairingAgentAdaptor.h"
#include "utils/Logger.h"

namespace MagicLink
{
class PairingAgent: public QObject
{
Q_OBJECT

public:
	explicit PairingAgent(QObject* parent = nullptr);
	~PairingAgent() override;

	bool start(const QString& pinCode);
	bool stop();

public slots:
	QString RequestPinCode(const QDBusObjectPath& device) const;

private:
	QString _pinCode;
	PairingAgentAdaptor* _adaptor;
	QDBusInterface* _agentManager;
	Logger* _log;

	bool _started;

	template<typename...Args>
	bool call(const QString& method, Args&& ...args);
};
}
