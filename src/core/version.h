#pragma once

#include <QString>

namespace tmog {

inline QString productName()
{
    return QStringLiteral(TMOG_PRODUCT_NAME);
}

inline QString version()
{
    return QStringLiteral(TMOG_VERSION);
}

inline QString versionLine()
{
    return productName() + QStringLiteral(" ") + version();
}

} // namespace tmog
