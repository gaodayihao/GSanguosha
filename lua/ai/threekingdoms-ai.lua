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
	
	local usehero_skill={}
	usehero_skill.name="usehero"
	table.insert(sgs.ai_skills,usehero_skill)
	usehero_skill.getTurnUseCard=function(self)
		local canInvoke = (self.player:getPile("heros"):length() > 1 and self.player:getMark("hero") > 0) or self.player:getMark("hero") == 0
		if self.player:getPile("heros"):isEmpty() or self.player:hasUsed("UseHeroCard") or not canInvoke then
			return 
		end
		
		if self.player:getGeneralName() ~= "anjiang" and math.random(0,2) == 0 then return end
		
		return sgs.Card_Parse("@UseHeroCard=.")
	end
	
	sgs.ai_skill_use_func.UseHeroCard = function(card, use, self)
		use.card = sgs.Card_Parse("@UseHeroCard=.")
	end
	
	sgs.ai_skill_use_func.PrepareCard = function(card, use, self)
		local cards=sgs.QList2Table(self.player:getHandcards())
		local heros={}
		
		for _, card in ipairs(cards) do
			if card:inherits("HeroCard") then
				table.insert(heros, card:getId())
			end	
		end
		if #heros > 0 then
			use.card = sgs.Card_Parse("@PrepareCard="..heros[math.random(1,#heros)])
		end
	end
	
	sgs.ai_skill_use["@prepare"]=function(self,prompt)
		local card=sgs.Card_Parse("@PrepareCard=.")
		local dummy_use={isDummy=true}
		self:useSkillCard(card,dummy_use)
		if dummy_use.card then return (dummy_use.card):toString() .. "->." end
		return "."
	end
	
	local herorecast_skill={}
	herorecast_skill.name="herorecast"
	table.insert(sgs.ai_skills,herorecast_skill)
	herorecast_skill.getTurnUseCard=function(self)
		local canInvoke = false
		local heros = self.player:getPile("heros")
		for _,hero in sgs.qlist(heros) do
			if sgs.Sanguosha:getCard(hero):hasFlag("justdraw") then
				canInvoke = true
				break
			end
		end
		
		if not canInvoke then return end
		
		if canInvoke and  not (heros:length() > 2) and math.random(0,1) == 0 then return end
		
		return sgs.Card_Parse("@RecastHeroCard=.")
	end
	
	sgs.ai_skill_use_func.RecastHeroCard = function(card, use, self)
		if self.player:getGeneralName() == "anjiang" then
			use.card = nil
			return
		end
		use.card = sgs.Card_Parse("@RecastHeroCard=.")
	end
	
	sgs.ai_skill_askforag.herorecast = function(self, card_ids)
		return card_ids[math.random(1,#card_ids)]
	end
	
	sgs.ai_skill_askforag.throwhero = function(self, card_ids)
		local myname = self.player:getGeneralName()
		for _, card_id in ipairs(card_ids) do
			if sgs.Sanguosha:getCard(card_id):objectName() ~= myname then
				return card_id
			end
		end
		return card_ids[math.random(1,#card_ids)]
	end
	
	sgs.ai_skill_askforag.throwSelfHero = sgs.ai_skill_askforag.throwhero
	
	sgs.ai_skill_askforag.throwTargetHero = function(self, card_ids)
		for _, card_id in ipairs(card_ids) do
			local targetname = sgs.Sanguosha:getCard(card_id):objectName()
			if self.player:getRoom():findPlayer(targetname) then
				return card_id
			end
		end
		return card_ids[math.random(1,#card_ids)]
	end
	
	sgs.ai_skill_choice.threekingdoms_mode = function(self, choice)
		return "throw"
	end
	
	sgs.ai_skill_choice.SnatchTargetHero = function(self, choice)
		return "snatch"
	end
end