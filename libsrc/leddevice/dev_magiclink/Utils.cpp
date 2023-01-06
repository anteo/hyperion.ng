#include "Utils.h"
#include <QList>

namespace MagicLink
{
QString qAsciiTable(const QList<QStringList>& rows)
{
	QList<int> lens;

	const QStringList& headers = rows[0];

	for (int col = 0; col < headers.size(); col++)
	{
		int maxLen = headers[col].size();

		for (int row = 1; row < rows.size(); row++)
		{
			int cellLen = rows[row][col].size();
			if (cellLen > maxLen)
			{
				maxLen = cellLen;
			}
		}

		lens.append(maxLen);
	}

	QStringList formats;
	for (int i = 0; i < lens.size(); i++)
	{
		formats << QString("%%1").arg(i + 1);
	}

	QStringList columnSeparators;
	for (int colLen: lens)
	{
		columnSeparators.append(QString().fill('-', colLen));
	}
	QString separators = columnSeparators.join("-+-");
	QString pattern = formats.join(" | ");

	QString table;

	for (int row = 0; row < rows.size(); row++)
	{
		QString rowPattern = pattern;

		for (int col = 0; col < lens.size(); col++)
		{
			int col_len = lens[col];
			QString cell = rows[row][col];

			rowPattern = rowPattern.arg(cell, -col_len);
		}

		table += " " + rowPattern + "\n";

		if (row == 0)
		{
			table += "-" + separators + "\n";
		}
	}
	return table;
}
}
