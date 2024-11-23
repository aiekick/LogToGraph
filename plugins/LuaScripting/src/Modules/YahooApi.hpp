#pragma once

/*
Date en format epoch

https://query1.finance.yahoo.com/v8/finance/chart/NVDA?interval=5m&period1=1438228800&period2=1440907200
https://query1.finance.yahoo.com/v8/finance/chart/NVDA?interval=5m&range=1y

Données historiques

URL de base : https://query1.finance.yahoo.com/v8/finance/chart/{SYMBOL}
Paramètres courants :
    symbol: le symbole de l’actif (par exemple, AAPL pour Apple)
    interval: intervalle de temps (ex. 1d pour quotidien, 1h pour chaque heure)
    range: période à récupérer (ex. 1mo pour un mois, 1y pour un an)
    events: inclut les événements comme les dividendes ou fractionnements (ex. div pour dividendes)
    Exemple d'URL :
    https://query1.finance.yahoo.com/v8/finance/chart/AAPL?interval=1d&range=1y&events=div

https://github.com/ranaroussi/yfinance/wiki/Ticker#history
https://yahooquery.dpguthrie.com/guide/ticker/historical/
*/

#include <array>
#include <string>
#include <sstream>

class YahooApi {
public:
    enum class Interval { _1m = 0, _2m, _5m, _15m, _30m, _60m, _90m, _1h, _1d, _5d, _1wk, _1mo, _3mo, Count };
    std::array<std::string, (size_t)Interval::Count> m_IntervalStrings = {"1m", "2m", "5m", "15m", "30m", "60m", "90m", "1h", "1d", "5d", "1wk", "1mo", "3mo"};
    std::array<int32_t, (size_t)Interval::Count> m_IntervalInMins = {1, 2, 5, 15, 30, 60, 90, 60, 60*24, 60*24*5, 60*24*7, 60*24*31, 60*24*312*3};

    enum class Range { _1d = 0, _5d, _1mo, _3mo, _6mo, _1y, _2y, _5y, _10y, _ytd, _max, Count };
    std::array<std::string, (size_t)Range::Count> m_RangeStrings = {"1d", "5d", "1mo", "3mo", "6mo", "1y", "2y", "5y", "10y", "ytd", "max"};
    
    using Date=time_t;

public:

    std::string getUrlForHistory(const std::string& vSymbol, Interval vInterval, Range vRange) {
        std::stringstream url;
        url << "https://query1.finance.yahoo.com/v8/finance/chart/" << vSymbol;
        url << "?interval=" << m_IntervalStrings[(size_t)vInterval];
        url << "&range=" << m_RangeStrings[(size_t)vRange];
        return url.str();
    }
    
    std::string getUrlForHistory(const std::string& vSymbol, Interval vInterval, Date vStartDate, Range vRange) {
        std::stringstream url;
        url << "https://query1.finance.yahoo.com/v8/finance/chart/" << vSymbol;
        url << "?interval=" << m_IntervalStrings[(size_t)vInterval];
        url << "&range=" << m_RangeStrings[(size_t)vRange];
        url << "&period2=" << vStartDate;
        return url.str();
    }
    
    std::string getUrlForHistory(const std::string& vSymbol, Interval vInterval, Date vStartDate, Date vEndDate) {
        std::stringstream url;
        url << "https://query1.finance.yahoo.com/v8/finance/chart/" << vSymbol;
        url << "?interval=" << m_IntervalStrings[(size_t)vInterval];
        url << "&period1=" << vStartDate;
        url << "&period2=" << vEndDate;
        return url.str();
    }
};
