#include "Connection.h"

#define HIBYTE(w) ((unsigned char)(((unsigned int)(w) >> 24) & 0xFF))

namespace MagicLink
{

static unsigned int crcTable[] = {
	0x0,
	0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
	0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6,
	0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
	0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
	0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F,
	0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A,
	0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
	0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58,
	0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033,
	0xA4AD16EA, 0xA06C0B5D, 0xD4326D90, 0xD0F37027, 0xDDB056FE,
	0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
	0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4,
	0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
	0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5,
	0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
	0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 0x7897AB07,
	0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C,
	0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1,
	0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
	0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B,
	0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698,
	0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D,
	0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
	0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F,
	0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
	0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80,
	0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
	0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
	0x58C1663D, 0x558240E4, 0x51435D53, 0x251D3B9E, 0x21DC2629,
	0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C,
	0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
	0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E,
	0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65,
	0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8,
	0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
	0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2,
	0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
	0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74,
	0x857130C3, 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
	0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21,
	0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A,
	0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 0x18197087,
	0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
	0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D,
	0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE,
	0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB,
	0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
	0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 0x89B8FD09,
	0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
	0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF,
	0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

Connection::Connection(QObject* parent)
	: QObject(parent)
	  , _pairingTimeout(new QTimer(this))
	  , _discoveryTimeout(new QTimer(this))
	  , _restartPairingTimeout(new QTimer(this))
	  , _restartConnectionTimeout(new QTimer(this))
	  , _restartDiscoveryTimeout(new QTimer(this))
	  , _configuration(new Configuration(this))
	  , _readBuffer(new QBuffer(this))
	  , _log(Logger::getInstance("MAGICLINK"))
{
	_pairingTimeout->setSingleShot(true);
	_discoveryTimeout->setSingleShot(true);
	_restartConnectionTimeout->setSingleShot(true);
	_restartPairingTimeout->setSingleShot(true);

	connect(_restartPairingTimeout, &QTimer::timeout, this, &Connection::startPairing);
	connect(_restartConnectionTimeout, &QTimer::timeout, this, &Connection::startConnection);
	connect(_restartDiscoveryTimeout, &QTimer::timeout, this, &Connection::startDiscovery);
	connect(_pairingTimeout, &QTimer::timeout, this, &Connection::pairingError);
	connect(_discoveryTimeout, &QTimer::timeout, this, &Connection::discoveryFinished);

	_localDevice = new QBluetoothLocalDevice(this);
	_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(_localDevice->address(), this);
	connect(_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &Connection::deviceDiscovered);
	connect(_discoveryAgent, static_cast<void (QBluetoothDeviceDiscoveryAgent::*)(
				QBluetoothDeviceDiscoveryAgent::Error)>(&QBluetoothDeviceDiscoveryAgent::error),
			this, &Connection::discoveryError);

	connect(_localDevice, &QBluetoothLocalDevice::pairingFinished, this, &Connection::pairingFinished);
	connect(_localDevice, &QBluetoothLocalDevice::error, this, &Connection::pairingError);

	_readBuffer->open(QBuffer::ReadWrite);
}

Connection::Connection(const QString& address, QObject* parent)
	: Connection(parent)
{
	setDeviceAddress(address);
}

void Connection::initiate()
{
	finish();
	startPairing();
}

void Connection::finish()
{
	if (_socket) _socket->close();
	_restartConnectionTimeout->stop();
	_restartPairingTimeout->stop();
	_restartDiscoveryTimeout->stop();
	_pairingTimeout->stop();
	_discoveryTimeout->stop();
	delete _pairingAgent;
	_pairingAgent = nullptr;
	_pairingErrorsCount = 0;
	_socketErrorsCount = 0;
	_discoveryErrorsCount = 0;
	_ready = false;
}

void Connection::startPairing()
{
	QBluetoothLocalDevice::Pairing status = _localDevice->pairingStatus(_deviceAddress);
	if (status == QBluetoothLocalDevice::Paired || status == QBluetoothLocalDevice::AuthorizedPaired)
	{
		Info(_log, "Device is already paired.");
		_localDevice->requestPairing(_deviceAddress, QBluetoothLocalDevice::Paired);
	}
	else
	{
		Info(_log, "Device is not yet paired.");
		if (_deviceName.isEmpty())
			startDiscovery();
		else
			deviceDiscovered(QBluetoothDeviceInfo(QBluetoothAddress(_deviceAddress), _deviceName, 0));
	}
}

void Connection::startDiscovery()
{
	Info(_log, "Starting device name discovery...");
	_discoveryTimeout->start(10000);
	_discoveryAgent->start();
}

void Connection::discoveryError()
{
	Error(_log, "Error occurred during discovery: %s", QSTRING_CSTR(_discoveryAgent->errorString()));
	_discoveryTimeout->stop();
	_discoveryAgent->stop();
	if (++_discoveryErrorsCount >= _maxErrors)
	{
		finish();
		emit connectionError(_discoveryAgent->errorString());
	}
	else
	{
		_restartDiscoveryTimeout->start(5000);
	}
}

void Connection::discoveryFinished()
{
	Error(_log, "Device name can not be obtained");
	_discoveryTimeout->stop();
	_discoveryAgent->stop();
	if (++_discoveryErrorsCount >= _maxErrors)
	{
		finish();
		emit connectionError("Device name can not be obtained");
	}
	else
	{
		_restartDiscoveryTimeout->start(5000);
	}
}

void Connection::deviceDiscovered(const QBluetoothDeviceInfo& info)
{
	if (info.address() != _deviceAddress) return;
	_discoveryTimeout->stop();
	_discoveryAgent->stop();
	_deviceName = info.name();
	Info(_log, "Device name discovered: %s", QSTRING_CSTR(info.name()));
	continuePairing();
}

void Connection::continuePairing()
{
	QString pinCode = calculatePinCode();
	Info(_log, "Requesting pairing using PIN %s ...", QSTRING_CSTR(pinCode));
	if (!pairingAgent()->start(pinCode))
	{
		pairingError();
	}
	else
	{
		_localDevice->requestPairing(_deviceAddress, QBluetoothLocalDevice::Paired);
		_pairingTimeout->start(30000);
	}
}

void Connection::pairingFinished(const QBluetoothAddress& address)
{
	delete _pairingAgent;
	_pairingAgent = nullptr;

	_pairingTimeout->stop();
	Info(_log, "Pairing finished.");
	startConnection();
}

void Connection::pairingError()
{
	Error(_log, "Error occurred during pairing");
	_pairingTimeout->stop();
	if (++_pairingErrorsCount >= _maxErrors)
	{
		finish();
		emit connectionError("Can't pair the device");
	}
	else
	{
		_restartPairingTimeout->start(5000);
	}
}

QString Connection::calculatePinCode()
{
	QByteArray id = _deviceName.split(' ').takeLast().toLatin1();
	unsigned int btPin = (id.at(2) * id.at(7)) ^ 0xD3;
	btPin ^= id.at(3) * id.at(1);
	btPin += id.at(4) | id.at(6) << 3;
	btPin ^= (unsigned char) ~(id.at(7) ^ 0x4C);
	btPin %= 9999;
	return QString::number(btPin).rightJustified(4, '0');
}

void Connection::startConnection()
{
	Info(_log, "Initiating connection to %s...", QSTRING_CSTR(_deviceAddress.toString()));
	delete _socket;
	_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
	connect(_socket, &QBluetoothSocket::connected, this, &Connection::socketConnected);
	connect(_socket, &QBluetoothSocket::readyRead, this, &Connection::socketReadyRead);
	connect(_socket, &QBluetoothSocket::bytesWritten, this, &Connection::packetSent);
	connect(_socket, static_cast<void (QBluetoothSocket::*)
		(QBluetoothSocket::SocketError)>(&QBluetoothSocket::error), this, &Connection::onSocketError);
	_socket->connectToService(_deviceAddress, 1, QBluetoothSocket::ReadWrite | QBluetoothSocket::Unbuffered);
}

void Connection::socketConnected()
{
	Info(_log, "Connected.");
	emit connected(_deviceAddress);
}

void Connection::socketReadyRead()
{
	if (_configuration->read(this))
	{
		if (_configuration->isModified())
		{
			Info(_log, "Detected configuration change:");
			Info(_log, "\n%s", QSTRING_CSTR(_configuration->printOut()));
			Info(_log, "Syncing...");
			_configuration->write(this);
		}
		else if (!_ready)
		{
			Info(_log, "MagicLink configuration:");
			Info(_log, "\n%s", QSTRING_CSTR(_configuration->printOut()));
			Info(_log, "Connection is ready.");
			_ready = true;
			emit ready();
		}
	}
}

void Connection::onSocketError()
{
	Error(_log, "Socket error: %s", QSTRING_CSTR(_socket->errorString()));
	_socket->abort();
	if (++_socketErrorsCount >= _maxErrors)
	{
		finish();
		emit socketError(_socket->errorString());
	}
	else
	{
		_restartConnectionTimeout->start(10000);
	}
}

bool Connection::receivePacket(QByteArray& packet)
{
	_readBuffer->write(_socket->readAll());
	qint64 size = _readBuffer->size();
	if (size != 10)
	{
		if (size > 10)
		{
			_readBuffer->buffer().clear();
			_readBuffer->reset();
		}
		return false;
	}
	_readBuffer->reset();
	QDataStream stream(_readBuffer);
	bool success = false;
	quint16 header;
	stream >> header;
	if (header == 0x04CA)
	{
		packet = _readBuffer->read(8);
		success = true;
	}
	_readBuffer->buffer().clear();
	_readBuffer->reset();
	return success;
}

void Connection::sendPacket(const QByteArray& packet, int size)
{
	if (size < 0) size = packet.size() + 3;

	QByteArray data(size, 0);

	data[0] = 0xAB;
	data[1] = 0xCD;
	data.replace(2, packet.size(), packet);
	data[size - 1] = 0xF5;

	//Debug(_log, "Write packet: %s", QSTRING_CSTR(QString(data.toHex())));
	_socket->write(data);
}

bool Connection::isReady() const
{
	return _ready;
}

Configuration* Connection::configuration() const
{
	return _configuration;
}

const Settings& Connection::actualSettings()
{
	return _configuration->actual();
}

const Settings& Connection::currentSettings()
{
	return _configuration->current();
}

bool Connection::isBufferOverrun() const
{
	return _socket->bytesToWrite() > 0;
}

void Connection::setMaxErrors(int maxErrors)
{
	_maxErrors = maxErrors;
}

Connection::~Connection()
{
	finish();
}

std::vector<ColorRgb> Connection::scaleColors(const std::vector<ColorRgb>& colors)
{
	std::vector<ColorRgb> scaledColors{};
	int count = static_cast<int>(colors.size());
	int scalingLevel = currentSettings().scalingLevel;
	if (scalingLevel <= 1 || currentSettings().firmwareVersion > 8)
		return colors;

	for (int i = 0; i < count; i += scalingLevel)
	{
		int r = 0, g = 0, b = 0;

		for (int j = 0; j < scalingLevel; j++)
		{
			int pos = (i + j) % count;
			r += colors[pos].red;
			g += colors[pos].green;
			b += colors[pos].blue;
		}

		r /= scalingLevel;
		g /= scalingLevel;
		b /= scalingLevel;

		scaledColors.emplace_back(static_cast<uint8_t>(r),
								  static_cast<uint8_t>(g),
								  static_cast<uint8_t>(b));
	}

	return scaledColors;
}

QByteArray Connection::colorsToByteArray(const std::vector<ColorRgb>& colors)
{
	QByteArray bytes;
	switch (currentSettings().colorBits)
	{
		case ColorBits::RGB888:
			for (ColorRgb color: colors)
			{
				uint8_t byte1 = color.green;
				uint8_t byte2 = color.red;
				uint8_t byte3 = color.blue;
				if (byte1 == 0xF5) byte1--;
				if (byte2 == 0xF5) byte2--;
				if (byte3 == 0xF5) byte3--;
				bytes.append(byte1);
				bytes.append(byte2);
				bytes.append(byte3);
			}
			break;
		case ColorBits::RGB555:
			for (ColorRgb color: colors)
			{
				uint8_t red = color.red >> 3;
				uint8_t green = color.green >> 3;
				uint8_t blue = color.blue >> 3;

				uint8_t byte1 = (green << 6) | red;
				uint8_t byte2 = (blue << 3) | (green >> 2);

				if (byte1 == 0xF5) byte1 ^= 0x20;
				if (byte2 == 0xF5) byte1 ^= 0x08;

				bytes.append(byte1);
				bytes.append(byte2);
			}
			break;
		default:
			break;
	}
	return bytes;
}

unsigned int Connection::calcCRC(const QByteArray& data)
{
	int len = data.size();
	int zeros = abs(4 - (len & 3)) & 3;
	unsigned int crc = -1;
	unsigned char mask = currentSettings().colorBits == ColorBits::RGB888 ? -16 : -1;

	for (int i = 0; i < len; ++i)
		crc = crcTable[mask & data.at(i) ^ HIBYTE(crc)] ^ (crc << 8);
	for (int j = 0; zeros > j; ++j)
		crc = crcTable[HIBYTE(crc)] ^ (crc << 8);

	return crc;
}

void Connection::sendColors(const std::vector<ColorRgb>& colors)
{
	if (!isReady() || isBufferOverrun()) return;

	QByteArray packet;
	Settings current{currentSettings()};
	Settings actual{actualSettings()};

	auto resizedColors = colors;
	resizedColors.resize(current.ledsCount, ColorRgb::BLACK);
	auto scaledColors = scaleColors(resizedColors);

	uint8_t confByte;
	if (actual.smoothingLevel <= 49)
	{
		confByte = actual.smoothingLevel & 0x7F;
		packet.append(0x11);
	}
	else
	{
		packet.append(0x10);
	}
	if (actual.useCRC == UseCRC::YES) confByte |= 0x80;
	packet.append(confByte);
	QByteArray bytes{colorsToByteArray(scaledColors)};
	packet.append(bytes);
	if (current.firmwareVersion > 4)
	{
		unsigned int crc = actual.useCRC == UseCRC::YES ? calcCRC(bytes) : 0;
		QByteArray crcBytes(reinterpret_cast<const char*>(&crc), sizeof(int));
		for (auto&& byte: crcBytes) if (byte == 0xF5) byte ^= 1;
		packet.append(crcBytes);
	}
	sendPacket(packet);
}

void Connection::setDeviceAddress(const QBluetoothAddress& address)
{
	_deviceAddress = address;
}

void Connection::setDeviceAddress(const QString& address)
{
	setDeviceAddress(QBluetoothAddress(address));
}

PairingAgent* Connection::pairingAgent()
{
	if (!_pairingAgent) _pairingAgent = new PairingAgent();
	return _pairingAgent;
}

}
