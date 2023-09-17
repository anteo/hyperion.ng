#pragma once

#include <QObject>
#include <QBuffer>
#include <QTimer>
#include <QBluetoothAddress>
#include <QBluetoothSocket>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include "utils/ColorRgb.h"
#include "utils/Logger.h"
#include "PairingAgent.h"
#include "Configuration.h"
#include "Settings.h"

namespace MagicLink
{
class Configuration;

class Connection: public QObject
{
Q_OBJECT
public:
	explicit Connection(QObject* parent = nullptr);
	explicit Connection(const QString& address, QObject* parent = nullptr);
	~Connection() override;

	void initiate();
	void finish();
	bool receivePacket(QByteArray& packet);
	void sendPacket(const QByteArray& packet, int size = -1);
	bool isReady() const;
	bool isBufferOverrun() const;
	Configuration* configuration() const;
	const Settings& actualSettings();
	const Settings& currentSettings();
	void setMaxErrors(int maxErrors);
	void setDeviceAddress(const QBluetoothAddress& address);
	void setDeviceAddress(const QString& address);
	void sendColors(const std::vector<ColorRgb>& colors);

private slots:
	void deviceDiscovered(const QBluetoothDeviceInfo& info);
	void discoveryFinished();
	void discoveryError();
	void pairingFinished(const QBluetoothAddress& address);
	void pairingError();
	void startPairing();
	void continuePairing();
	void startDiscovery();
	void startConnection();
	void socketConnected();
	void onSocketError();
	void socketReadyRead();

signals:
	void connected(const QBluetoothAddress& address);
	void connectionError(const QString& message);
	void socketError(const QString& message, bool isRecoverable);
	void ready();
	void packetSent();

private:
	QString calculatePinCode();
	std::vector<ColorRgb> scaleColors(const std::vector<ColorRgb> &colors);
	QByteArray colorsToByteArray(const std::vector<ColorRgb> &colors);
	unsigned int calcCRC(const QByteArray &array);
	PairingAgent* pairingAgent();

	int _pairingErrorsCount = 0;
	int _socketErrorsCount = 0;
	int _discoveryErrorsCount = 0;
	int _maxErrors = 5;
	bool _ready = false;

	QBluetoothAddress _deviceAddress;
	QString _deviceName;
	Configuration* _configuration;
	QBluetoothLocalDevice* _localDevice;
	QBluetoothDeviceDiscoveryAgent* _discoveryAgent;
	QBluetoothUuid _serviceUUID;
	QTimer* _pairingTimeout;
	QTimer* _discoveryTimeout;
	QTimer* _restartPairingTimeout;
	QTimer* _restartConnectionTimeout;
	QTimer* _restartDiscoveryTimeout;
	PairingAgent* _pairingAgent{};
	QBluetoothSocket* _socket{};
	QBuffer* _readBuffer;
	Logger* _log;
};
}
