/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#undef LOG_MODULE
#define LOG_MODULE "REPORT"

#include <Arduino.h>
#include <Client.h>
#include <console.h>

#include "InfluxReporter.h"

using namespace reporter;

size_t InfluxReporter::send(uint32_t timestamp)
{
    if (mMeasurement.empty() || (mHeaders.empty() && mTags.empty() && mFields.empty()))
        return 0;

    if (!mClient_p->connect(mHost.c_str(), mPort))
        return 0;

    char metric[160] = {0};
    char *npos = metric;

    auto append = [&](auto &&...args) -> bool
    {
        if (static_cast<size_t>(npos - metric) >= sizeof(metric))
        {
            LOGE("failed to send metric, out of metric");
            return false;
        }
        npos += snprintf(npos, (sizeof(metric) - (npos - metric)),
                             std::forward<decltype(args)>(args)...);
        return true;
    };

    if (!append(mMeasurement.c_str()))
        return 0;

    for (auto it = mHeaders.begin(); it != mHeaders.end(); ++it)
    {
        if (!append(",%s=%s", std::get<NAME>(*it).c_str(), std::get<VALUE>(*it).c_str()))
            return 0;
    }

    for (auto it = mTags.begin(); it != mTags.end(); ++it)
    {
        if (!append(",%s=%s", std::get<NAME>(*it).c_str(), std::get<VALUE>(*it).c_str()))
            return 0;
    }

    for (auto it = mFields.begin(); it != mFields.end(); ++it)
    {
        if (!append("%c", (it == mFields.begin()) ? ' ' : ','))
            return 0;
        if (!append("%s=%s", std::get<NAME>(*it).c_str(), std::get<VALUE>(*it).c_str()))
            return 0;
    }

    if ((timestamp != 0) && (timestamp != static_cast<uint32_t>(-1)))
    {
        if (!append(" %s000000000", std::to_string(timestamp).c_str()))
            return 0;
    }

    const size_t length = mClient_p->write(
        reinterpret_cast<const uint8_t *>(metric),
        strlen(metric));

    LOGI(metric);

    mClient_p->flush();
    mClient_p->stop();

    return length;
}
