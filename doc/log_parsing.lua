--LogInfo(infos_string) 					 -- will log the message in the in app console 
--LogWarning(warning_string) 				 -- will log the message in the in app console 
--LogError(error_string) 					 -- will log the message in the in app console 
--SetScriptDescription(description_string)	 -- will set the description of your script in app
--SetRowBufferName("buffer_row");			 -- set the lua string varaible name who will be filled with the content of the row file
--SetFunctionForEachRow("eachRow");			 -- set the function name who will be called at each row of the file
--SetFunctionForEndFile("endFile");			 -- set the function name who will be called at the end of the file
--GetRowIndex()								 -- return the row number of the file
--GetRowCount()								 -- return the number of rows of the file
--Init() 									 -- is the entry point of the script. this function is needed
--GetEpochTime("2023-01-16 15:24:26,464", 0) -- get epoch time from datetime in format "YYYY-MM-DD HH:MM:SS,MS" or "YYYY-MM-DD HH:MM:SS.MS" with hour offset in second param
--AddSignalTag(date, r, g, b, a, name, help) -- add a signal tag with date, color a name. the help will be displayed when mouse over the tag
--AddSignalStatus(signal_category, signal_name, signal_epoch_time, signal_status)

--will add a signal numerical value 
--AddSignalValue(signal_category, signal_name, signal_epoch_time, signal_value)
--will add a signal start zone
--AddSignalStartZone(signal_category, signal_name, signal_epoch_time, signal_string)
--will add a signal end zone
--AddSignalEndZone(signal_category, signal_name, signal_epoch_time, signal_string)

function Init()
  	SetInfos("sample script")
	SetRowBufferName("buffer_row");
	SetFunctionForEachRow("eachRow");
	SetFunctionForEndFile("endFile");
	
	LogInfo(" --- Start of file parsing ---");
end

function eachRow()
	_section, _time, _name, _value = string.match(buffer_row, "<profiler section=\"(.*)\" epoch_time=\"(.*)\" name=\"(.*)\" render_time_ms=\"(.*)\">")
	if _section ~= nil and _time ~= nil and _name ~= nil and _value ~= nil then
		AddSignalValue(_section, _name, _time, _value)
	end
end

function endFile()
	LogInfo(" --- End of file parsing ---");
end
