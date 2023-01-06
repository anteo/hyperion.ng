#pragma once

#include <QObject>

namespace MagicLink
{
Q_NAMESPACE

enum class UseCRC
{
	UNSET = -1,
	YES,
	NO
};
Q_ENUM_NS(UseCRC)

enum class LedType
{
	UNSET = -1,
	RGBW,
	RGB
};
Q_ENUM_NS(LedType)

enum class ColorBits
{
	UNSET = -1,
	RGB555,
	RGB888
};
Q_ENUM_NS(ColorBits)

enum class ColorCurve
{
	UNSET = -1,
	NORMAL_CURVE,
	CIE_CURVE,
	S_CURVE,
	MIX_CURVE
};
Q_ENUM_NS(ColorCurve)

struct Settings
{
	UseCRC useCRC{UseCRC::UNSET};
	LedType ledType{LedType::UNSET};
	ColorBits colorBits{ColorBits::UNSET};
	ColorCurve colorCurve{ColorCurve::UNSET};
	int ledsCount{-1};
	int firmwareVersion{-1};
	int smoothingLevel{-1};
	int sync{-1};
	int scalingLevel{-1};
	int advancedSettings{-1};
};
}
