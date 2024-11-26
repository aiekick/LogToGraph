# Lua Scripting

```lua
-- UserDatas ltg (LogToGraph valid only from LogToGraph)
-- ltg:logInfo(infos_string) : will log the message in the in app console 				
-- ltg:logWarning(infos_string) : will log the message in the in app console 
-- ltg:logError(infos_string) : will log the message in the in app console 
-- ltg:logDebug(infos_string) : will log the message in the in app console 
-- ltg:addSignalTag(date, r, g, b, a, name, help) : add a signal tag with date, color a name (color is linear [0:1]. the help will be displayed when mouse over the tag
-- ltg:addSignalStatus(signal_category, signal_name, signal_epoch_time, signal_status) : will add a signal string status
-- ltg:addSignalValue(signal_category, signal_name, signal_epoch_time, signal_value) : will add a signal numerical value
-- ltg:addSignalStartZone(signal_category, signal_name, signal_epoch_time, signal_string) : will add a signal start zone
-- ltg:addSignalEndZone(signal_category, signal_name, signal_epoch_time, signal_string) : will add a signal end zone 
-- ltg:getRowCount() -- get row count in the file
-- ltg:getRowIndex() -- get row index in the file
-- get/set epoch time from datetime in format "YYYY-MM-DD HH:MM:SS,MS" or "YYYY-MM-DD HH:MM:SS.MS" with hour offset in second param
-- double ltg:stringToEpoch("2023-01-16 15:24:26,464", 0)   
-- string ltg:epochToString(18798798465465.546546, 0)

function startFile()

end

local x = 0.0;
function parse(buffer)
	x = x + 0.01 
	ltg:addSignalValue("curve", "cos", x, math.cos(x))
	ltg:addSignalValue("curve", "sin", x, math.sin(x))
end

function endFile()

end
```