-- This is the start script of QSanguosha

package.path = package.path .. ";./lua/lib/?.lua"
package.cpath = package.cpath .. ";./lua/clib/?.dll"

dofile "lua/sgs_ex.lua"

function load_translation(file)
	local t = dofile(file)
	if type(t) ~= "table" then
	    error(("file %s is should return a table!"):format(file))
	end
	
	sgs.LoadTranslationTable(t)
end

function load_translations()
	local lang = sgs.GetConfig("Language", "zh_CN")
	local lang_dir = "lang/" .. lang

	local lang_files = sgs.GetFileNames(lang_dir)
	for _, file in ipairs(lang_files) do	
		load_translation(("%s/%s"):format(lang_dir, file))
	end
end

function load_extensions(just_require)
	local scripts = sgs.GetFileNames("extensions")
	
	for _, script in ipairs(scripts) do	
		if script:match(".+%.lua$") then
			local name = script:sub(script:find("%w+"))
			local module_name = "extensions." .. name
			local loaded = require(module_name)
			
			sgs.Sanguosha:addPackage(loaded.extension)
		end
	end
end

load_extensions()

local done_loading = sgs.Sanguosha:property("DoneLoading"):toBool()
if not done_loading then
	load_translations()
	done_loading = sgs.QVariant(true)
	sgs.Sanguosha:setProperty("DoneLoading", done_loading)
end