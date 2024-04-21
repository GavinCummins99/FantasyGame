// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimGraphNode_NarrativeDialogue.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"

#define LOCTEXT_NAMESPACE "NarrativeDialogueNode"

UAnimGraphNode_NarrativeDialogue::UAnimGraphNode_NarrativeDialogue(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_NarrativeDialogue::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_NarrativeDialogue", "Narrative Dialogue Head Controller");
}

FText UAnimGraphNode_NarrativeDialogue::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_NarrativeDialogue_Tooltip", "This alters the head bone to make dialogue avatars either look at who they are speaking to, or look at who is speaking depending on their state.");
}

FText UAnimGraphNode_NarrativeDialogue::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText NodeTitle;
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		NodeTitle = GetControllerDescription();
	}
	else
	{
		NodeTitle = FText(LOCTEXT("AnimGraphNode_NarrativeDialogue_Title", "Narrative Dialogue Controller"));
	}
	return NodeTitle;
}

void UAnimGraphNode_NarrativeDialogue::ValidateAnimNodePostCompile(class FCompilerResultsLog& MessageLog, class UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{

}

bool UAnimGraphNode_NarrativeDialogue::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph);
}

#undef LOCTEXT_NAMESPACE
