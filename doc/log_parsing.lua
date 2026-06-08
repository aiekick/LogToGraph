-- UserDatas ltg (LogToGraph valid only from LogToGraph)
-- ltg:logInfo(infos_string) : will log the message in the in app console
-- ltg:logWarning(infos_string) : will log the message in the in app console
-- ltg:logError(infos_string) : will log the message in the in app console
-- ltg:logDebug(infos_string) : will log the message in the in app console
-- ltg:addSignalTag(date, r, g, b, a, name, help) : add a signal tag with date, color a name (color is linear [0:1]. the help will be displayed when mouse over the tag
-- ltg:addSignalStatus(signal_category, signal_name, signal_epoch_time, signal_status) : will add a signal string status
-- ltg:addSignalValue(signal_category, signal_name, signal_epoch_time, signal_value, description_string_optional) : will add a signal numerical value
-- ltg:addSignalStartZone(signal_category, signal_name, signal_epoch_time, signal_string) : will add a signal start zone
-- ltg:addSignalEndZone(signal_category, signal_name, signal_epoch_time, signal_string) : will add a signal end zone
-- get/set epoch time from datetime in format "YYYY-MM-DD HH:MM:SS,MS" or "YYYY-MM-DD HH:MM:SS.MS" with hour offset in second param
-- double ltg:stringToEpoch("2023-01-16 15:24:26,464", 0)
-- string ltg:epochToString(18798798465465.546546, 0)
-- regex (boost::regex engine, PCRE-like syntax — supports |, {m,n}, lookahead, etc.):
-- local re = ltg:regex(pattern)  -- compile once at script init, reuse on every row
-- re:test(input)            -> bool
-- re:match(input)           -> captures (multiple values), or nil
-- re:find(input)            -> start, end (1-based), or nil
-- re:gsub(input, repl)      -> result, count (mirrors string.gsub)
-- re:gmatch(input)          -> iterator (for use in `for cap in re:gmatch(s) do ... end`)

function startFile(filepath)
	ltg:logInfo(" --- Start parsing of file '" .. filepath .. "'");
end

local re = ltg:regex([[<profiler section="([^"]*)" epoch_time="([^"]*)" name="([^"]*)" render_time_ms="([^"]*)">]])

function parse(buffer)
    local section, time, name, value = re:match(buffer)
    if section and time and name and value then
    	local epoch = ltg:stringToEpoch(time, 0);
        ltg:addSignalValue(section, name, epoch, tonumber(value))
    end
end

function endFile(filepath)
	ltg:logInfo(" --- End parsing of file '" .. filepath .. "'");
end
