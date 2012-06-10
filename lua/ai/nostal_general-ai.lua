local nosfanjian_skill={}
nosfanjian_skill.name="nosfanjian"
table.insert(sgs.ai_skills,nosfanjian_skill)
nosfanjian_skill.getTurnUseCard=function(self)
	if self.player:isKongcheng() then return nil end
	if self.player:usedTimes("NosFanjianCard")>0 then return nil end

	local cards = self.player:getHandcards()

	for _, card in sgs.qlist(cards) do
		if card:getSuit() == sgs.Card_Diamond and self.player:getHandcardNum() == 1 then
			return nil
		elseif card:inherits("Peach") or card:inherits("Analeptic") then
			return nil
		end
	end

	local card_str = "@NosFanjianCard=."
	local fanjianCard = sgs.Card_Parse(card_str)
	assert(fanjianCard)

	return fanjianCard
end

sgs.ai_skill_use_func.NosFanjianCard = sgs.ai_skill_use_func.FanjianCard

sgs.ai_card_intention.NosFanjianCard = sgs.ai_card_intention.FanjianCard

sgs.dynamic_value.damage_card.NosFanjianCard = true

sgs.ai_chaofeng.noszhouyu = sgs.ai_chaofeng.zhouyu

sgs.ai_skill_invoke.noslieren = sgs.ai_skill_invoke.lieren

sgs.ai_skill_pindian.noslieren = sgs.ai_skill_pindian.lieren

sgs.ai_skill_cardask["@enyuanheart"] = function(self)
	local cards = self.player:getHandcards()
	for _, card in sgs.qlist(cards) do
		if card:getSuit() == sgs.Card_Heart and not (card:inherits("Peach") or card:inherits("ExNihilo")) then
			return card:getEffectiveId()
		end
	end
	return "."
end

function sgs.ai_slash_prohibit.nosenyuan(self)
	if self:isWeak() then return true end
end

nosxuanhuo_skill={}
nosxuanhuo_skill.name="nosxuanhuo"
table.insert(sgs.ai_skills,nosxuanhuo_skill)
nosxuanhuo_skill.getTurnUseCard=function(self)
	if not self.player:hasUsed("NosXuanhuoCard") then
		return sgs.Card_Parse("@NosXuanhuoCard=.")
	end
end

sgs.ai_skill_use_func.NosXuanhuoCard = function(card, use, self)
	local cards = self.player:getHandcards()
	cards=sgs.QList2Table(cards)
	self:sortByUseValue(cards,true)

	local target
	for _, friend in ipairs(self.friends_noself) do
		if self:hasSkills(sgs.lose_equip_skill, friend) and not friend:getEquips():isEmpty() then
			for _, card in ipairs(cards) do
				if card:getSuit() == sgs.Card_Heart and self.player:getHandcardNum() > 1 then
					use.card = sgs.Card_Parse("@NosXuanhuoCard=" .. card:getEffectiveId())
					target = friend
					break
				end
			end
		end
		if target then break end
	end
	if not target then
		for _, enemy in ipairs(self.enemies) do
			if not enemy:isKongcheng() then
				for _, card in ipairs(cards)do
					if card:getSuit() == sgs.Card_Heart and not card:inherits("Peach")  and self.player:getHandcardNum() > 1 then
						use.card = sgs.Card_Parse("@NosXuanhuoCard=" .. card:getEffectiveId())
						target = enemy
						break
					end
				end
			end
			if target then break end
		end
	end

	if target then
		self.room:setPlayerFlag(target, "xuanhuo_target")
		if use.to then
			use.to:append(target)
		end
	end
end

sgs.ai_skill_playerchosen.nosxuanhuo = function(self, targets)
	for _, player in sgs.qlist(targets) do
		if (player:getHandcardNum() <= 2 or player:getHp() < 2) and self:isFriend(player) and not player:hasFlag("xuanhuo_target") then
			return player
		end
	end
end

sgs.nosfazheng_suit_value = 
{
	heart = 3.9
}

sgs.ai_chaofeng.nosfazheng = -3

nosjujian_skill={}
nosjujian_skill.name="nosjujian"
table.insert(sgs.ai_skills,nosjujian_skill)
nosjujian_skill.getTurnUseCard=function(self)
	if not self.player:hasUsed("NosJujianCard") then return sgs.Card_Parse("@NosJujianCard=.") end
end

sgs.ai_skill_use_func.NosJujianCard = function(card, use, self)
	local abandon_handcard = {}
	local index = 0
	local hasPeach = (self:getCardsNum("Peach") > 0)

	local trick_num, basic_num, equip_num = 0, 0, 0
	if not hasPeach and self.player:isWounded() and self.player:getHandcardNum() >=3 then
		local cards = self.player:getCards("he")
		cards=sgs.QList2Table(cards)
		self:sortByUseValue(cards, true)
		for _, card in ipairs(cards) do
			if card:getTypeId() == sgs.Card_Trick and not card:inherits("ExNihilo") then trick_num = trick_num + 1
			elseif card:getTypeId() == sgs.Card_Basic then basic_num = basic_num + 1
			elseif card:getTypeId() == sgs.Card_Equip then equip_num = equip_num + 1
			end
		end
		local result_class
		if trick_num >= 3 then result_class = "TrickCard"
		elseif equip_num >= 3 then result_class = "EquipCard"
		elseif basic_num >= 3 then result_class = "BasicCard"
		end
		local f
		for _, friend in ipairs(self.friends_noself) do
			if (friend:getHandcardNum()<2) or (friend:getHandcardNum()<friend:getHp()+1) then
				for _, fcard in ipairs(cards) do
					if fcard:inherits(result_class) and not fcard:inherits("ExNihilo") then
						table.insert(abandon_handcard, fcard:getId())
						index = index + 1
					end
					if index == 3 then f = friend break end
				end
			end
		end
		if index == 3 then
			if use.to then use.to:append(f) end
			use.card = sgs.Card_Parse("@NosJujianCard=" .. table.concat(abandon_handcard, "+"))
			return
		end
	end
	abandon_handcard = {}
	local cards = self.player:getHandcards()
	cards=sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local slash_num = self:getCardsNum("Slash")
	local jink_num = self:getCardsNum("Jink")
	for _, friend in ipairs(self.friends_noself) do
		if (friend:getHandcardNum()<2) or (friend:getHandcardNum()<friend:getHp()+1) or self.player:isWounded() then
			for _, card in ipairs(cards) do
				if #abandon_handcard >= 3 then break end
				if not card:inherits("Nullification") and not card:inherits("EquipCard") and
					not card:inherits("Peach") and not card:inherits("Jink") and
					not card:inherits("Indulgence") and not card:inherits("SupplyShortage") then
					table.insert(abandon_handcard, card:getId())
					index = 5
				elseif card:inherits("Slash") and slash_num > 1 then
					if (self.player:getWeapon() and not self.player:getWeapon():objectName()=="crossbow") or
						not self.player:getWeapon() then
						table.insert(abandon_handcard, card:getId())
						index = 5
						slash_num = slash_num - 1
					end
				elseif card:inherits("Jink") and jink_num > 1 then
					table.insert(abandon_handcard, card:getId())
					index = 5
					jink_num = jink_num - 1
				end
			end
			if index == 5 then
				use.card = sgs.Card_Parse("@NosJujianCard=" .. table.concat(abandon_handcard, "+"))
				if use.to then use.to:append(friend) end
				return
			end
		end
	end
	if #self.friends_noself>0 and self:getOverflow()>0 then
		self:sort(self.friends_noself, "handcard")
		local discard = self:askForDiscard("gamerule", math.min(self:getOverflow(),3))
		use.card = sgs.Card_Parse("@NosJujianCard=" .. table.concat(discard, "+"))
		if use.to then use.to:append(self.friends_noself[1]) end
		return
	end
end

sgs.ai_use_priority.NosJujianCard = 4.5
sgs.ai_use_value.NosJujianCard = 6.7

sgs.ai_card_intention.NosJujianCard = -100

sgs.dynamic_value.benefit.NosJujianCard = true

sgs.ai_skill_choice.nosxuanfeng = function(self, choices)
	self:sort(self.enemies, "defense")
	local slash = sgs.Card_Parse(("slash[%s:%s]"):format(sgs.Card_NoSuit, 0))
	for _, enemy in ipairs(self.enemies) do
		if self.player:distanceTo(enemy)<=1 then
			return "damage"
		elseif not self:slashProhibit(slash ,enemy) then
			return "slash"
		end
	end
	return "nothing"
end

sgs.ai_skill_playerchosen.xuanfeng_damage = sgs.ai_skill_playerchosen.damage

sgs.ai_skill_playerchosen.xuanfeng_slash = sgs.ai_skill_playerchosen.zero_card_as_slash

sgs.ai_skill_invoke.noszhenggong = function(self,data)
	local target = data:toPlayer()
	
	if self:isFriend(target) then
		return (self:hasSkills(sgs.lose_equip_skill, target) and not self:isWeak(target)) or (self:isEquip("SilverLion", target) and target:isWounded())
	end
	
	return true
end

function sgs.ai_cardneed.noszhenggong(to, card, self)
	if not to:containsTrick("indulgence") and to:getMark("nosbaijiang") == 0 then
		return card:getTypeId() == sgs.Card_Equip
	end
end

sgs.ai_skill_cardchosen.noszhenggong = function(self, who, flags)
	for i = 0, 3 do
		if not self.player:getEquip(i) and who:getEquip(i) then
			return who:getEquip(i)
		end
	end
	
	return sgs.Sanguosha:getCard(self:askForCardChosenDefault(who, flags))
end

sgs.ai_skill_use["@@nosquanji"] = function(self, prompt)
	local current = self.room:getCurrent()
	if self:isFriend(current) then
		if current:hasSkill("zhiji") and not current:hasSkill("guanxing") and current:getHandcardNum() == 1 then
			return "@NosQuanjiCard=" .. self:getMinCard(self.player):getId() .. "->."
		end
	elseif self:isEnemy(current) then
		if self.player:getHandcardNum() <= 1 and not self:needKongcheng(self.player) then return "." end
		local invoke = false
		if current:hasSkill("yinghun") and current:getLostHp() > 2 then invoke = true end
		if current:hasSkill("luoshen") and not self:isWeak() then invoke = true end
		if current:hasSkill("baiyin") and not current:hasSkill("jilve") and current:getMark("@bear") >= 4 then invoke = true end
		if current:hasSkill("zaoxian") and not current:hasSkill("jixi") and current:getPile("field"):length() >= 3 then invoke = true end
		if current:hasSkill("zili") and not current:hasSkill("paiyi") and current:getPile("power"):length() >= 3 then invoke = true end
		if current:hasSkill("hunzi") and not current:hasSkill("yingzi") and current:getHp() == 1 then invoke = true end
		if self:isWeak(current) and self.player:getHandcardNum() > 1 and current:getCards("j"):isEmpty() then invoke = true end
		
		if invoke and self:getMaxCard(self.player):getNumber() > 7 then
			return "@NosQuanjiCard=" .. self:getMaxCard(self.player):getId() .. "->."
		end
	end
	
	return "."
end

sgs.ai_skill_invoke.nosyexin = function(self,data)
	return true
end

local nosyexin_skill={}
nosyexin_skill.name="nosyexin"
table.insert(sgs.ai_skills, nosyexin_skill)
nosyexin_skill.getTurnUseCard = function(self)
	if self.player:getPile("nospower"):isEmpty() or self.player:hasUsed("NosYexinCard") then
		return
	end
	
	return sgs.Card_Parse("@NosYexinCard=.")
end

sgs.ai_skill_use_func.NosYexinCard = function(card, use, self)
	use.card = sgs.Card_Parse("@NosYexinCard=.")
end

sgs.ai_skill_askforag.nosyexin = function(self, card_ids)
	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	self:sortByCardNeed(cards)
	return cards[#cards]:getEffectiveId()
end

sgs.ai_skill_askforag.nospaiyi = function(self, card_ids)
	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	self:sortByCardNeed(cards)
	
	for _, acard in ipairs(cards) do
		if acard:inherits("Indulgence") or acard:inherits("SupplyShortage") then
			sgs.nosPaiyiCard = acard
			return acard:getEffectiveId()
		end
	end
	
	local card = cards[#cards]
	sgs.nosPaiyiCard = card
	return card:getEffectiveId()
end

local function hp_subtract_handcard(a,b)
	local diff1 = a:getHp() - a:getHandcardNum()
	local diff2 = b:getHp() - b:getHandcardNum()

	return diff1 < diff2
end

local function handcard_subtract_hp(a, b)
	local diff1 = a:getHandcardNum() - a:getHp()
	local diff2 = b:getHandcardNum() - b:getHp()

	return diff1 < diff2
end

sgs.ai_skill_playerchosen.nospaiyi = function(self, targets)

	if sgs.nosPaiyiCard:inherits("Indulgence") then
		table.sort(self.enemies, hp_subtract_handcard)
		
		local enemies = self.enemies
		for _, enemy in ipairs(enemies) do
			if self:hasSkills("lijian|fanjian|nosfanjian",enemy) and not enemy:containsTrick("indulgence") and not enemy:isKongcheng() and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end
		
		for _, enemy in ipairs(enemies) do
			if not enemy:containsTrick("indulgence") and not enemy:hasSkill("keji") and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end
	end
	
	if sgs.nosPaiyiCard:inherits("SupplyShortage") then
		table.sort(self.enemies, handcard_subtract_hp)
		
		local enemies = self.enemies
		for _, enemy in ipairs(enemies) do
			if (self:hasSkills("yongsi|haoshi|tuxi", enemy) or (enemy:hasSkill("zaiqi") and enemy:getLostHp() > 1)) and
				not enemy:containsTrick("supply_shortage") and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end
		for _, enemy in ipairs(enemies) do
			if ((#enemies == 1) or not self:hasSkills("tiandu|guidao",enemy)) and not enemy:containsTrick("supply_shortage") and enemy:faceUp() and self:objectiveLevel(enemy) > 3 then
				sgs.nosPaiyiTarget = enemy
				sgs.nosPaiyiCard = nil
				return enemy
			end
		end
	end
	
	if sgs.nosPaiyiCard:inherits("Shit") then
		table.sort(self.enemies, handcard_subtract_hp)
		sgs.nosPaiyiTarget = self.enemies[1]
		sgs.nosPaiyiCard = nil
		return self.enemies[1]
	end
	
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	for _, target in ipairs(targets) do
		if self:isEnemy(target) and target:hasSkill("zhiji") and not target:hasSkill("guanxing") and target:getHandcardNum() == 0 then
			sgs.nosPaiyiTarget = target
			sgs.nosPaiyiCard = nil
			return target 
		end 
	end
	
	for _, target in ipairs(targets) do
		if self:isFriend(target) and target:objectName() ~= self.player:objectName() then
			sgs.nosPaiyiTarget = target
			sgs.nosPaiyiCard = nil
			return target 
		end 
	end
	
	sgs.nosPaiyiTarget = self.player
	sgs.nosPaiyiCard = nil
	return self.player
end

sgs.ai_skill_choice.nospaiyi = function(self, choices)
	local choice_table = choices:split("+")
	if table.contains(choice_table, "Judging") and self:isEnemy(sgs.nosPaiyiTarget) then
		sgs.nosPaiyiTarget = nil
		return "Judging"
	end
	
	if table.contains(choice_table, "Equip") and self:isFriend(sgs.nosPaiyiTarget) then
		sgs.nosPaiyiTarget = nil
		return "Equip"
	end
	
	sgs.nosPaiyiTarget = nil
	return "Hand"
end
