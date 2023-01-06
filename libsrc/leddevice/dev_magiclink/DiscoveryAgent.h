#pragma once

#include <functional>
#include <QObject>
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>
#include <QBluetoothUuid>
#include <QTimer>
#include "utils/Logger.h"

namespace MagicLink
{

class DiscoveryAgent: public QObject
{
Q_OBJECT

public:
	explicit DiscoveryAgent(QObject* parent = nullptr);
	~DiscoveryAgent();

	void start();
	void stop();

	QBluetoothDeviceDiscoveryAgent* discoveryAgent() const;
	QList<QBluetoothDeviceInfo>* acceptedDevices() const;

	int roundsCount() const;
	int errorsCount() const;
	bool isStarted() const;

	static QString deviceDescription(const QBluetoothDeviceInfo& info);
	void setMaxRounds(int maxRounds);
	void setMaxErrors(int maxErrors);
	void setTimeoutSec(int timeoutSec);
	void cleanupCachedDevices();

private slots:
	void handleFinished();
	void handleDeviceDiscovered(const QBluetoothDeviceInfo& info);
	void handleDeviceUpdated(const QBluetoothDeviceInfo& info);
	void handleError();
	void nextRound();

signals:
	void deviceDiscovered(const QBluetoothDeviceInfo&) const;
	void finished() const;
	void error(QString);

private:
	QBluetoothDeviceDiscoveryAgent* _discoveryAgent{};
	QList<QBluetoothDeviceInfo>* _acceptedDevices;
	QList<QBluetoothAddress>* _devicesForCleanup;
	QBluetoothLocalDevice* _localDevice{};
	int _roundsCount{}, _errorsCount{};
	int _maxRounds = 3, _maxErrors = 10;
	QTimer* _timeoutTimer;
	QTimer* _restartTimer;
	Logger* _log;
	bool _started{false};
	int _timeoutSec = 60;

	bool removeDevice(const QBluetoothAddress& address);
	bool checkDeviceAccepted(const QBluetoothDeviceInfo& info);
	bool isAccepted(const QBluetoothDeviceInfo& info);
};

}
