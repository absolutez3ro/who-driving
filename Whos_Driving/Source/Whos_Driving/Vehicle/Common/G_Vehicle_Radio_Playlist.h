#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

// A shuffle bag avoids repeats within a cycle; history makes Previous reversible.
class FGVehicleRadioPlaylist
{
public:
	void Reset(const TArray<FSoftObjectPath>& tracks, const FSoftObjectPath& current = FSoftObjectPath())
	{
		available = tracks;
		bag.Reset();
		history.Reset();
		cursor = INDEX_NONE;
		random.GenerateNewSeed();
		if (available.Contains(current))
		{
			history.Add(current);
			cursor = 0;
			bag = available;
			bag.Remove(current);
			for (int32 index = bag.Num() - 1; index > 0; --index)
			{
				bag.Swap(index, random.RandRange(0, index));
			}
		}
	}

	FSoftObjectPath Next()
	{
		if (history.IsValidIndex(cursor + 1))
		{
			return history[++cursor];
		}
		if (available.IsEmpty())
		{
			return FSoftObjectPath();
		}
		if (bag.IsEmpty())
		{
			bag = available;
			for (int32 index = bag.Num() - 1; index > 0; --index)
			{
				bag.Swap(index, random.RandRange(0, index));
			}
			if (bag.Num() > 1 && history.IsValidIndex(cursor) && bag.Last() == history[cursor])
			{
				bag.Swap(0, bag.Num() - 1);
			}
		}
		const FSoftObjectPath track = bag.Pop(EAllowShrinking::No);
		if (history.Num() == 128)
		{
			history.RemoveAt(0, 1, EAllowShrinking::No);
		}
		history.Add(track);
		cursor = history.Num() - 1;
		return track;
	}

	FSoftObjectPath Previous()
	{
		if (cursor > 0)
		{
			--cursor;
		}
		return history.IsValidIndex(cursor) ? history[cursor] : FSoftObjectPath();
	}

private:
	TArray<FSoftObjectPath> available;
	TArray<FSoftObjectPath> bag;
	TArray<FSoftObjectPath> history;
	FRandomStream random;
	int32 cursor = INDEX_NONE;
};
