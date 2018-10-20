#include "iEquip_Events.h"

#include "GameAPI.h"  // g_thePlayer
#include "GameEvents.h"  // EventResult, EventDispatcher
#include "GameObjects.h"  // TESObjectWEAP
#include "GameTypes.h"  // BSFixedString
#include "ITypes.h"  // UInt32, SInt32
#include "PapyrusArgs.h"  // PackValue()
#include "PapyrusEvents.h"  // EventRegistration, SKSEModCallbackEvent, NullParameters, RegistrationSetHolder, NullParameters
#include "PapyrusVM.h"  // Output, VMClassRegistry, IFunctionArguments

#include "iEquip_ActorExtLib.h"  // GetEquippedHand
#include "RE_GameEvents.h"  // RE::TESEquipEvent


#if _WIN64
// Moved from PapyrusVM in SSE
#include "PapyrusValue.h"  // VMValue
#endif


namespace iEquip_Events
{
	template <typename T> void SetVMValue(VMValue* a_val, T a_arg)
	{
		VMClassRegistry * registry = (*g_skyrimVM)->GetClassRegistry();
		PackValue(a_val, &a_arg, registry);
	}


	template <> void SetVMValue<bool>(VMValue * val, bool arg) { val->SetBool(arg); }
	template <> void SetVMValue<SInt32>(VMValue * val, SInt32 arg) { val->SetInt(arg); }
	template <> void SetVMValue<float>(VMValue * val, float arg) { val->SetFloat(arg); }
	template <> void SetVMValue<BSFixedString>(VMValue * val, BSFixedString arg) { val->SetString(arg.data); }


	template <typename T1>
	class EventQueueFunctor1 : public IFunctionArguments
	{
	public:
		EventQueueFunctor1(BSFixedString& a_eventName, T1 a_arg1) :
			eventName(a_eventName.data),
			arg1(a_arg1)
		{}


		virtual bool Copy(Output* a_dst) override
		{
			a_dst->Resize(1);
			SetVMValue(a_dst->Get(0), arg1);

			return true;
		}


		void operator() (const EventRegistration<NullParameters>& a_reg)
		{
			VMClassRegistry* registry = (*g_skyrimVM)->GetClassRegistry();
			registry->QueueEvent(a_reg.handle, &eventName, this);
		}


	private:
		BSFixedString eventName;
		T1 arg1;
	};


	template <typename T1, typename T2>
	class EventQueueFunctor2 : public IFunctionArguments
	{
	public:
		EventQueueFunctor2(BSFixedString & a_eventName, T1 a_arg1, T2 a_arg2) :
			eventName(a_eventName.data),
			arg1(a_arg1),
			arg2(a_arg2)
		{}

		virtual bool Copy(Output* a_dst)
		{
			a_dst->Resize(2);
			SetVMValue(a_dst->Get(0), arg1);
			SetVMValue(a_dst->Get(1), arg2);

			return true;
		}

		void operator() (const EventRegistration<NullParameters>& a_reg)
		{
			VMClassRegistry * registry = (*g_skyrimVM)->GetClassRegistry();
			registry->QueueEvent(a_reg.handle, &eventName, this);
		}

	private:
		BSFixedString	eventName;
		T1				arg1;
		T2				arg2;
	};


	EquipEventHandler::~EquipEventHandler()
	{}


	EventResult EquipEventHandler::ReceiveEvent(RE::TESEquipEvent* a_event, EventDispatcher<RE::TESEquipEvent>* a_dispatcher)
	{
		TESObjectWEAP* weap = 0;
		if (a_event->akSource != *g_thePlayer) {
			return kEvent_Continue;
		} else if (weap = a_event->checkIfBoundWeapEquipEvent()) {
			if (!a_event->isUnequipWeaponArmorEvent()) {
				static BSFixedString callbackName = "OnBoundWeaponEquipped";
				UInt32 equipSlots = iEquip_ActorExt::getEquippedSlots((*g_thePlayer), weap);
				g_boundWeaponEquippedCallbackRegs.ForEach(EventQueueFunctor2<UInt32, UInt32>(callbackName, weap->gameData.type, equipSlots));
				_DMESSAGE("[DEBUG] OnBoundWeaponEquipped event dispatched\n");
			} else {
				static BSFixedString callbackName = "OnBoundWeaponUnequipped";
				UInt32 unequipSlots = iEquip_ActorExt::getUnequippedSlots((*g_thePlayer));
				g_boundWeaponUnequippedCallbackRegs.ForEach(EventQueueFunctor2<TESObjectWEAP*, UInt32>(callbackName, weap, unequipSlots));
				_DMESSAGE("[DEBUG] OnBoundWeaponUnequipped event dispatched\n");
			}
		}
		return kEvent_Continue;
	}


	RegistrationSetHolder<NullParameters> g_boundWeaponEquippedCallbackRegs;
	RegistrationSetHolder<NullParameters> g_boundWeaponUnequippedCallbackRegs;
	EquipEventHandler g_equipEventHandler;
}