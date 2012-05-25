
sgs.ai_skill_invoke.longpo = function(self, data)
	if math.random(0, 1) == 0 then return true else return false end
end

sgs.ai_skill_invoke.hengmao = sgs.ai_skill_invoke.longpo

sgs.ai_skill_invoke.fengfa = sgs.ai_skill_invoke.longpo

sgs.ai_skill_invoke.weixue = sgs.ai_skill_invoke.longpo

local longhunEx_skill={}
longhunEx_skill.name="longhunEx"
table.insert(sgs.ai_skills, longhunEx_skill)
longhunEx_skill.getTurnUseCard = function(self)
	local cards=sgs.QList2Table(self.player:getCards("he"))
	self:sortByUseValue(cards,true)
	for _, card in ipairs(cards) do
		if card:getSuit() == sgs.Card_Diamond then
			return sgs.Card_Parse(("fire_slash:longhunEx[%s:%s]=%d"):format(card:getSuitString(),card:getNumberString(),card:getId()))
		end
	end
end

sgs.ai_view_as.longhunEx = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card:getSuit() == sgs.Card_Diamond then
		return ("fire_slash:longhunEx[%s:%s]=%d"):format(suit, number, card_id)
	elseif card:getSuit() == sgs.Card_Club then
		return ("jink:longhunEx[%s:%s]=%d"):format(suit, number, card_id)
	elseif card:getSuit() == sgs.Card_Heart then
		return ("peach:longhunEx[%s:%s]=%d"):format(suit, number, card_id)
	elseif card:getSuit() == sgs.Card_Spade then
		return ("nullification:longhunEx[%s:%s]=%d"):format(suit, number, card_id)
	end
end

sgs.ai_skill_invoke.duojianQG = function(self, data)
	for _,friend in ipairs(self.friends) do
		if self:isEquip("QinggangSword", friend) and not friend:hasSkill("xuanfeng") and not(friend:hasSkill("xiaoji") and friend:getHandcardNum()<3) then
			return false
		end
	end
	for _,enemy in ipairs(self.enemies) do
		local cards=sgs.QList2Table(enemy:getJudgingArea())
		for _, card in ipairs(cards) do
			if card:inherits("QinggangSword") then return false end
		end
		if self:isEquip("QinggangSword", enemy) and ((enemy:hasSkill("xiaoji") and enemy:getHandcardNum()<3) or enemy:hasSkill("xuanfeng")) then
			return false
		end
	end
	return true
end

sgs.chudaishenzhaoyun_suit_value = 
{
	heart = 6.7,
	spade = 5,
	club = 4.2,
	diamond = 3.9,
}