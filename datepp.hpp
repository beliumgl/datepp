/*
 * Date and Time Manager for Unix-Timestamps
 * written in C++.
 *
 * Has support for negative timestamps! :D
 *
 * Single-header. By beliumgl.
 */

#pragma once

#include <string>
#include <stdexcept>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

/*
 * Because the `unsigned char` type is not commonly used,
 * readers may not be familiar with it, so I have provided placeholders for them.
 *
 * The unsigned char type is well-suited for this context because none of these values
 * will exceed 255 and cannot be negative.
 */
using month_t = unsigned char;
using day_t = unsigned char;
using hour_t = unsigned char;
using minute_t = unsigned char;
using second_t = unsigned char;
using timezone_offset_t = double;
using year_t = short;

namespace beliumgl {
    class DateTimeFormat {
    private:
        /*
         * Examples for date 01.01.1970:
         * -----------------------------------------------------------------
         * order = "MDY": "1/1/1970",
         * order = "YMD": "1970/1/1",
         * delimiter = '/': "1/1/1970",
         * delimiter = '.': "1.1.1970",
         * alphabeticalMonth = true: "Jan 1 1970",
         * showTime = true: "Jan 1 1970 00:00:00",
         * showDotw = true: "Thu, Jan 1 1970 00:00:00",
         * fillZeros = true: "Thu, Jan 01 1970 00:00:00",
         * showUTCoffset = true: "Thu, Jan 01 1970 00:00:00 +00 UTC",
         * _12Hours = true: "Thu, Jan 01 1970 12:00:00 AM +00 UTC",
         * fullNames = true: "Thursday, January 01 1970 12:00:00 AM +00 UTC"
         * -----------------------------------------------------------------
         *
         * Default format: "Thu, 01/01/1970 00:00:00 +00 UTC"
         */

        char delimiter = '/';
        bool showDotw = false, showTime = false, showUTCoffset = false, fillZeros = false, alphabeticalMonth = false, _12Hours = false, fullNames = false;
        std::string order = "mdy";

        /*
         * -------
         * HELPERS
         * -------
         */
        inline std::string toLowercase(std::string str) {
            try {
                std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
                return str;
            } catch (...) {
                throw std::runtime_error("Failed to `lowercase` a string.");
            }
        }

        inline void removeAll(std::string& str, char token) {
            str.erase(std::remove(str.begin(), str.end(), token), str.end());
        }

        inline void removeDuplicates(std::string& str) {
            std::unordered_set<char> seen;
            std::string output;
            output.reserve(str.size());
            for (char c : str) {
                if (seen.insert(c).second) {
                    output.push_back(c);
                }
            }
            str = std::move(output);
        }
    public:
        /*
         * How does my string format work?
         *
         * It is not case-sensitive, and unknown characters are ignored (except when they are used as delimiters).
         * The delimiter will be the last character that separates D, M (or A), and Y.
         *
         * The code uses several tokens: W, D, M, Y, H, I, S, O, and special tokens: _ and A.
         * ------------------------------------------
         * W - day of the week
         * D - day
         * M - month
         * Y - year
         * H - hour
         * I - minute (set to 'i' because 'm' is taken)
         * S - second
         * O - UTC offset
         * _ - 12-hour format
         * A - alphabetical month
         * ------------------------------------------
         * If the pattern is `WW`, it means to show full names.
         * If the patterns are `DD`, `MM`, or `YY`, it means to fill with zeros.
         * If there is only one token, it will be interpreted as short names or without leading zeros.
         *
         * The order of D, M (or A), and Y is important (e.g., MM/DD/YY and DD/MM/YY will be interpreted differently).
         *
         * H, I, S, W, O, _, and A are not required in the format, but omitting them will result in the loss of some information, such as time, UTC offset, time format, etc.
         */
        DateTimeFormat(char* format) : DateTimeFormat(std::string(format)) {}
        DateTimeFormat(const std::string& format = "W, DD/MM/YY, HH:II:SS O UTC");
        DateTimeFormat(char delimiter = '/',
                       bool showDotw = true,
                       bool showTime = true,
                       bool showUTCoffset = true,
                       bool fillZeros = true,
                       bool alphabeticalMonth = false,
                       bool _12Hours = false,
                       bool fullNames = false,
                       const std::string& order = "mdy")
        : delimiter(delimiter), showDotw(showDotw),
        showTime(showTime), showUTCoffset(showUTCoffset),
        fillZeros(fillZeros), alphabeticalMonth(alphabeticalMonth),
        _12Hours(_12Hours), fullNames(fullNames), order(toLowercase(order)) {}

        char getDelimiter() const { return this->delimiter; }
        bool getShowDotw() const { return this->showDotw; }
        bool getShowTime() const { return this->showTime; }
        bool getShowUTCoffset() const { return this->showUTCoffset; }
        bool getFillZeros() const { return this->fillZeros; }
        bool getAlphabeticalMonth() const { return this->alphabeticalMonth; }
        bool get12HourFormat() const { return this->_12Hours; }
        bool getFullNames() const { return this->fullNames; }
        std::string getOrder() const { return this->order; }
    };

    class DateTime {
    private:
        // Store constructor inputs for easier conversion back to unix timestamp.
        char* unix_lit;
        std::string unix_str;

        // Day Of The Week (or DOTW).
        enum class DOTW {
            Sunday = 0, Monday = 1, Tuesday = 2, Wednesday = 3, Thursday = 4, Friday = 5, Saturday = 6
        };
        const std::unordered_map<DOTW, std::string> dotwStrMap {
            {DOTW::Sunday, "Sunday"},
            {DOTW::Monday, "Monday"},
            {DOTW::Tuesday, "Tuesday"},
            {DOTW::Wednesday, "Wednesday"},
            {DOTW::Thursday, "Thursday"},
            {DOTW::Friday, "Friday"},
            {DOTW::Saturday, "Saturday"}
        };
        const std::unordered_map<month_t, std::string> monthsStrMap {
            {0, "January"},
            {1, "February"},
            {2, "March"},
            {3, "April"},
            {4, "May"},
            {5, "June"},
            {6, "July"},
            {7, "August"},
            {8, "September"},
            {9, "October"},
            {10, "November"},
            {11, "December"},
        };

        /*
         * Those default values represent `Thu, 01/01/1970 00:00:00 +00 UTC`.
         *
         * Don't worry if you see days and months as 0;
         * they'll still be interpreted as 'first', like array indexes.
         */
        const year_t defYears = 1970;
        const month_t defMonth = 0;
        const day_t defDays = 0;
        const hour_t defHours = 0;
        const minute_t defMinutes = 0;
        const second_t defSeconds = 0;
        const timezone_offset_t defTimezoneOffset = 0;
        const size_t shortStrLength = 3;

        year_t years = 1970;
        month_t months = 0;
        DOTW dotw = DOTW::Thursday;
        day_t days = 0;
        hour_t hours = 0;
        minute_t minutes = 0;
        second_t seconds = 0;
        timezone_offset_t timezoneOffset = 0.0;

        /*
         * -------
         * HELPERS
         * -------
         */
        template<typename T>
        inline std::string padZeros(T num) {
            return (num < 10 && num >= 0 ? "0" : "") + std::to_string(static_cast<T>(num));
        }

        inline bool isLeapYear(year_t year) {
            return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
        }

        day_t daysInMonth(year_t year, month_t month) {
            if (month < 0 || month > 11)
                throw std::invalid_argument("Invalid month (must be 0-11)");

            static constexpr std::array<day_t, 12> days =
            {31,28,31,30,31,30,31,31,30,31,30,31};
            return (month == 1 && isLeapYear(year)) ? 29 : days[month];
        }

         DOTW dotwByDate(year_t year, month_t month, day_t day) {
            if (month < 1 || month > 12)
                throw std::invalid_argument("Month must be 1-12");

            day_t max_day = daysInMonth(year, month - 1);
            if (day < 1 || day > max_day)
                throw std::invalid_argument("Invalid day for month");

            int m = month;
            int y = year;
            if (m < 3) {
                m += 12;
                y -= 1;
            }

            int q = day;
            int K = y % 100;
            int J = y / 100;

            int h = (q + 13*(m + 1)/5 + K + K/4 + J/4 + 5*J) % 7;
            return static_cast<DOTW>((h - 1 + 7) % 7);
        }

        void parseUnix(long long _unix, timezone_offset_t timezoneOffset) {
            constexpr int secondsInDay = 86400;
            constexpr int secondsInHour = 3600;
            constexpr int secondsInMinute = 60;

            long long timezoneSeconds = static_cast<long long>(timezoneOffset * secondsInHour);
            long long adjustedUnix = _unix + timezoneSeconds;
            long long days = adjustedUnix / secondsInDay;
            int remainderSeconds = static_cast<int>(adjustedUnix % secondsInDay);

            if (remainderSeconds < 0) {
                remainderSeconds += secondsInDay;
                days -= 1;
            }

            year_t year = this->defYears;
            if (days >= 0) {
                while (true) {
                    unsigned short daysInYear = isLeapYear(year) ? 366 : 365;
                    if (days >= daysInYear) {
                        days -= daysInYear;
                        ++year;
                    } else break;
                }
            } else {
                while (days < 0) {
                    year_t prevYear = year - 1;
                    unsigned short daysInYear = isLeapYear(prevYear) ? 366 : 365;
                    days += daysInYear;
                    --year;
                }
            }

            month_t month = this->defMonth;
            while (true) {
                day_t monthDays = daysInMonth(year, month);
                if (days >= monthDays) {
                    days -= monthDays;
                    ++month;
                } else break;
            }

            int hours = remainderSeconds / secondsInHour;
            remainderSeconds %= secondsInHour;
            int minutes = remainderSeconds / secondsInMinute;
            int seconds = remainderSeconds % secondsInMinute;

            this->years = year;
            this->months = month;
            this->days = days;
            this->hours = hours;
            this->minutes = minutes;
            this->seconds = seconds;
            this->dotw = dotwByDate(year, month + 1, days + 1);
        }
    public:
        /*
         * The class supports an initalization with string literals
         * and string class from C++ STL.
         *
         * Optionally, you can explicitly provide timezone offset (you’ll still be able to use different offsets).
         */

        /*
         * ----------------------------------------------
         * DECLARATIONS (and one line implementations :D)
         * ----------------------------------------------
         */
        DateTime(char* _unix, timezone_offset_t timezoneOffset = 0.0);
        DateTime(const std::string& _unix, timezone_offset_t timezoneOffset = 0.0);

        char* toStringLit(char* format = "W, DD/MM/YY, HH:II:SS O UTC");
        char* toStringLit(const std::string& format = "W, DD/MM/YY, HH:II:SS O UTC"); // Return a string in specified format
        char* toStringLit(const DateTimeFormat& format = "W, DD/MM/YY, HH:II:SS O UTC");
        char* toUnixLit() const { return this->unix_lit; };
        std::string toString(char* format = "W, DD/MM/YY, HH:II:SS O UTC");
        std::string toString(const std::string& format = "W, DD/MM/YY, HH:II:SS O UTC"); // Return a string in specified format
        std::string toString(const DateTimeFormat& format = "W, DD/MM/YY, HH:II:SS O UTC");
        std::string toUnix() const { return this->unix_str; };

        year_t year() const { return this->years; };
        month_t month() const { return this->months; };
        DOTW dayOfTheWeekend() const { return this->dotw; };
        std::string dayOfTheWeekendStr(bool full = false) const;
        day_t day() const  { return this->days; };
        hour_t hour() const { return this->hours; };
        minute_t minute() const { return this->minutes; };
        second_t second() const { return this->seconds; };
        timezone_offset_t offsetUTC() const { return this->timezoneOffset; };

        /*
         * Rename of the same functions
         */
        DOTW dotwEnum() const { return this->dotw; };
        std::string dotwStr(bool full = false) const;

        /*
         * --------------------
         * OPERATOR OVERLOADING
         * --------------------
         */
        bool operator<(const DateTime& other) const;
        bool operator>(const DateTime& other) const;
        bool operator==(const DateTime& other) const;
        bool operator<=(const DateTime& other) const;
        bool operator>=(const DateTime& other) const;
        DateTime operator+(const DateTime& other) const;
        DateTime operator-(const DateTime& other) const;
        DateTime operator*(const DateTime& other) const;
        DateTime operator/(const DateTime& other) const;
    };

    /*
     * ---------------
     * IMPLEMENTATIONS
     * ---------------
     */
    DateTime::DateTime(char* _unix, timezone_offset_t timezoneOffset)
    : unix_lit(_unix), unix_str(std::string(_unix)), timezoneOffset(timezoneOffset) {
        try {
            parseUnix(std::strtoll(_unix, nullptr, 10), timezoneOffset);
        } catch (...) {
            throw std::invalid_argument("Your unix timestamp is invalid.");
        }
    }

    DateTime::DateTime(const std::string& _unix, timezone_offset_t timezoneOffset)
    : unix_lit(const_cast<char*>(_unix.data())), unix_str(_unix), timezoneOffset(timezoneOffset) {
        try {
            parseUnix(std::stoll(_unix), timezoneOffset);
        } catch (...) {
            throw std::invalid_argument("Your unix timestamp is invalid.");
        }
    }

    DateTimeFormat::DateTimeFormat(const std::string& format) {
        /*
         * I left a comment explaining how my format works in DateTimeFormat class,
         * so you can read it and understand how this code functions.
         */
        constexpr char dotwToken = 'w', dayToken = 'd', monthToken = 'm', yearToken = 'y',
        hourToken = 'h', minuteToken = 'i', secondToken = 's',
        timezoneOffsetToken = 'o', _12HoursToken = '_', alphabeticalMonthToken = 'a';

        std::string newFormat = toLowercase(format);
        std::string order;

        removeAll(newFormat, ' ');

        auto isOrderToken = [&](char c) {
            return c == dayToken || c == monthToken || c == yearToken || c == alphabeticalMonthToken;
        };

        for (size_t i = 0; i < newFormat.length(); ++i) {
            char token = newFormat[i];

            if (token == dotwToken) {
                this->showDotw = true;
                if (i + 1 < newFormat.length() && newFormat[i + 1] == dotwToken)
                    this->fullNames = true;
                continue;
            }

            if (isOrderToken(token)) {
                order.push_back(token);

                if (i + 1 < newFormat.length() && newFormat[i + 1] == token && !this->fillZeros)
                    this->fillZeros = true;
                if (order.size() < 3 && i + 1 < newFormat.length())
                    this->delimiter = newFormat[i + 1];

                if (token == alphabeticalMonthToken)
                    this->alphabeticalMonth = true;

                continue;
            }

            if (token == hourToken || token == minuteToken || token == secondToken) {
                this->showTime = true;
                if (i + 1 < newFormat.length() && newFormat[i + 1] == token && !this->fillZeros)
                    this->fillZeros = true;
                continue;
            }

            if (token == _12HoursToken) {
                this->_12Hours = true;
                continue;
            }

            if (token == timezoneOffsetToken) {
                this->showUTCoffset = true;
                continue;
            }
        }

        removeDuplicates(order);
        if (order.length() != 3)
            throw std::runtime_error("Failed to generate order from string.");
        this->order = order;
    }

    std::string DateTime::toString(const DateTimeFormat& format){
        constexpr char dayToken = 'd', monthToken = 'm', yearToken = 'y', alphabeticalMonthToken = 'a';
        std::string result;

        if (format.getShowDotw())
            result += format.getFullNames()
                ? this->dotwStrMap.at(this->dotw) + ", "
                : this->dotwStrMap.at(this->dotw).substr(0, this->shortStrLength) + ", ";

        std::string order = format.getOrder();
        for (size_t i = 0; i < order.length(); ++i) {
            switch (order[i]) {
                case dayToken:
                    result += format.getFillZeros()
                        ? padZeros(this->days + 1)
                        : std::to_string(this->days + 1);
                    break;
                case monthToken:
                    result += format.getFillZeros()
                        ? padZeros(this->months + 1)
                        : std::to_string(this->months + 1);
                    break;
                case alphabeticalMonthToken:
                    result += format.getFullNames()
                        ? this->monthsStrMap.at(this->months)
                        : this->monthsStrMap.at(this->months).substr(0, this->shortStrLength);
                    break;
                case yearToken:
                    result += std::to_string(this->years);
                    break;
            }
            result += format.getDelimiter();
        }
        result[result.length() - 1] = ' '; // Replace the last delimiter with space

        if (format.getShowTime()) {
            bool am = this->hours < 12;
            hour_t hours12 = this->hours % 12;
            if (hours12 == 0) hours12 = 12;

            if (format.get12HourFormat()) {
                result += format.getFillZeros()
                    ? padZeros(hours12) + ":"
                    : std::to_string(hours12) + ":";
            } else {
                result += format.getFillZeros()
                    ? padZeros(this->hours) + ":"
                    : std::to_string(this->hours) + ":";
            }
            result += format.getFillZeros() ? padZeros(this->minutes) + ":" : std::to_string(this->minutes) + ":";
            result += format.getFillZeros() ? padZeros(this->seconds) + " " : std::to_string(this->seconds) + " ";

            if (format.get12HourFormat()) {
                result += am ? "AM " : "PM ";
            }
        }

        if (format.getShowUTCoffset()) {
            bool isPositive = this->timezoneOffset >= 0;

            result += isPositive ? "+" : "";
            result += format.getFillZeros() ? padZeros(this->timezoneOffset) + " UTC" : std::to_string(this->timezoneOffset) + " UTC";
        }

        return result;
    }

    char* DateTime::toStringLit(char* format) {
        std::string tmp = toString(DateTimeFormat(format));
        char* buf = new char[tmp.size() + 1];
        std::copy(tmp.begin(), tmp.end(), buf);
        buf[tmp.size()] = '\0';
        return buf;
    }

    char* DateTime::toStringLit(const std::string& format) {
        std::string tmp = toString(DateTimeFormat(format));
        char* buf = new char[tmp.size() + 1];
        std::copy(tmp.begin(), tmp.end(), buf);
        buf[tmp.size()] = '\0';
        return buf;
    }

    char* DateTime::toStringLit(const DateTimeFormat& format) {
        std::string tmp = toString(format);
        char* buf = new char[tmp.size() + 1];
        std::copy(tmp.begin(), tmp.end(), buf);
        buf[tmp.size()] = '\0';
        return buf;
    }

    std::string DateTime::toString(char* format) {
        return toString(DateTimeFormat(format));
    }

    std::string DateTime::toString(const std::string& format) {
        return toString(DateTimeFormat(format));
    }

    std::string DateTime::dayOfTheWeekendStr(bool full) const {
        if (static_cast<unsigned char>(this->dotw) > 6)
            throw std::invalid_argument("Invalid day of the week.");

        const std::string& result = this->dotwStrMap.at(this->dotw);

        if (!full && this->shortStrLength > result.length())
            throw std::runtime_error("Short length is larger than the actual string.");

        return full ? result : result.substr(0, this->shortStrLength);
    }

    std::string DateTime::dotwStr(bool full) const {
        return dayOfTheWeekendStr(full);
    }

    /*
     * --------------------
     * OPERATOR OVERLOADING
     * --------------------
     */
    // Since Unix timestamps are already in UTC, we can just compare them.
    inline bool DateTime::operator<(const DateTime& other) const {
        return std::stoll(this->unix_str) < std::stoll(other.toUnix());
    }

    inline bool DateTime::operator>(const DateTime& other) const {
         return std::stoll(this->unix_str) > std::stoll(other.toUnix());
    }

    inline bool DateTime::operator==(const DateTime& other) const {
         return std::stoll(this->unix_str) == std::stoll(other.toUnix());
    }

    inline bool DateTime::operator<=(const DateTime& other) const {
        return std::stoll(this->unix_str) <= std::stoll(other.toUnix());
    }

    inline bool DateTime::operator>=(const DateTime& other) const {
        return std::stoll(this->unix_str) >= std::stoll(other.toUnix());
    }

    inline DateTime DateTime::operator+(const DateTime& other) const {
        return DateTime(std::to_string(std::stoll(this->unix_str) + std::stoll(other.toUnix())), 0.0);
    }

    inline DateTime DateTime::operator-(const DateTime& other) const {
        return DateTime(std::to_string(std::stoll(this->unix_str) - std::stoll(other.toUnix())), 0.0);
    }

    inline DateTime DateTime::operator*(const DateTime& other) const {
        return DateTime(std::to_string(std::stoll(this->unix_str) * std::stoll(other.toUnix())), 0.0);
    }

    inline DateTime DateTime::operator/(const DateTime& other) const {
        return DateTime(std::to_string(std::stoll(this->unix_str) / std::stoll(other.toUnix())), 0.0);
    }
}
