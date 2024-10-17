#include "StaffCraftingMenu.CategoryListEntry.h"

namespace UI
{
	void StaffCraftingMenu::CategoryListEntry::SetupEntryObject(RE::GFxValue& a_entryObj) const
	{
		a_entryObj.SetMember("filterFlag", filterFlag);
		a_entryObj.SetMember("enabled", enabled);
		a_entryObj.SetMember("equipState", selected ? 1 : 0);
		SetupEntryObjectByType(a_entryObj);
	}

	void StaffCraftingMenu::ItemEntry::ShowInItemCard(StaffCraftingMenu* a_menu) const
	{
		a_menu->UpdateItemCard(data.get());
	}

	void StaffCraftingMenu::ItemEntry::ShowItem3D(bool a_show) const
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		if (a_show) {
			inventory3D->UpdateItem3D(data.get());
		}
		else {
			inventory3D->Clear3D();
		}
	}

	const char* StaffCraftingMenu::ItemEntry::GetName() const
	{
		return data ? data->GetDisplayName() : "";
	}

	void StaffCraftingMenu::ItemEntry::SetupEntryObjectByType(RE::GFxValue& a_entryObj) const
	{
		a_entryObj.SetMember("text", GetName());
		a_entryObj.SetMember("count", data->countDelta);
	}

	void StaffCraftingMenu::SpellEntry::ShowInItemCard(StaffCraftingMenu* a_menu) const
	{
		a_menu->UpdateItemCard(data);
	}

	void StaffCraftingMenu::SpellEntry::ShowItem3D(bool a_show) const
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		if (a_show && data) {
			inventory3D->UpdateMagic3D(const_cast<RE::SpellItem*>(data), 0);
		}
		else {
			inventory3D->Clear3D();
		}
	}

	const char* StaffCraftingMenu::SpellEntry::GetName() const
	{
		return data ? data->GetFullName() : "";
	}

	void StaffCraftingMenu::SpellEntry::SetupEntryObjectByType(RE::GFxValue& a_entryObj) const
	{
		a_entryObj.SetMember("text", GetName());
	}

	void StaffCraftingMenu::RecipeEntry::ShowInItemCard(StaffCraftingMenu* a_menu) const
	{
		const auto createdItem = data->createdItem
			? data->createdItem->As<RE::TESBoundObject>()
			: nullptr;

		const auto item = std::make_unique<RE::InventoryEntryData>(
			createdItem,
			data->data.numConstructed);

		a_menu->UpdateItemCard(item.get());
	}

	void StaffCraftingMenu::RecipeEntry::ShowItem3D(bool a_show) const
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		if (a_show) {
			inventory3D->UpdateMagic3D(data->createdItem, 0);
		}
		else {
			inventory3D->Clear3D();
		}
	}

	const char* StaffCraftingMenu::RecipeEntry::GetName() const
	{
		return data ? data->createdItem->GetName() : "";
	}

	void StaffCraftingMenu::RecipeEntry::SetupEntryObjectByType(RE::GFxValue& a_entryObj) const
	{
		a_entryObj.SetMember("text", GetName());
		a_entryObj.SetMember("count", data->data.numConstructed);
	}

}
