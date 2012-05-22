if sgs.GetConfig("GameMode", "08p") == "03_3kingdoms" then
	sgs.isRolePredictable = function()
		return false
	end
	
	SmartAI.objectiveLevel = function(self, player)
		if self.player:objectName() == player:objectName() then return -5 end
		if self.player:getNextAlive():objectName() == player:objectName() then return 5 end
		return -2
	end
	
	SmartAI.isFriend = function(self, player)
		return self:objectiveLevel(player) < 0
	end

	SmartAI.isEnemy = function(self, player)
		return self:objectiveLevel(player) >= 0
	end
end