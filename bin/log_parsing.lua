-- logInfo(infos_string) 					 -- will log a message with level INFOS in the in app console 
-- logWarning(warning_string) 				 -- will log a message with level WARNING in app console 
-- logError(error_string) 					 -- will log a message with level ERROR in app console 

-- getRowIndex()						     -- return the row number of the file
-- getRowCount()							 -- return the number of rows of the file

-- stringToEpoch(string epoch, double hour_offset)
-- epochToString(Epoch epoch_time, double hour_offset)

-- add a signal tag with date, color a name. the help will be displayed when mouse over the tag
-- addSignalTag(Epoch date, double r, double g, double b, double a, string name, string help)

-- addSignalStatus(string signal_category, string signal_name, Epoch signal_epoch_time, string signal_status)
-- addSignalValue(string signal_category, string signal_name, Epoch signal_epoch_time, double signal_value)
-- addSignalStartZone(string signal_category, string signal_name, Epoch signal_epoch_time, string signal_string)
-- addSignalEndZone(string signal_category, string signal_name, Epoch signal_epoch_time, string signal_string)

function startFile()
	ltg:logInfo(" --- Start of file parsing ---");
end

function parse(buffer)
	_section, _time, _name, _value = string.match(buffer, "<profiler section=\"(.*)\" epoch_time=\"(.*)\" name=\"(.*)\" render_time_ms=\"(.*)\">")
	if _section ~= nil and _time ~= nil and _name ~= nil and _value ~= nil then
		print("_section : ", type(_section))
		epoch = ltg:stringToEpoch(_time, 0)
		print("_time : ", type(epoch))
		print("_name : ", type(_name))
		print("_value : ", type(_value))
		-- ltg:addSignalValue(_section, _name, ltg:stringToEpoch(_time, 0), _value)
	end
end

function endFile()
	ltg:logInfo(" --- End of file parsing ---");
end
