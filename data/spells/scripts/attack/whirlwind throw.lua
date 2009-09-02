local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_BLOCKARMOR, true)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_WEAPONTYPE)
setCombatParam(combat, COMBAT_PARAM_USECHARGES, true)
setCombatFormula(combat, COMBAT_FORMULA_SKILL, -0.33, 0, -1, 0)


function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
