package main

import (
	"fmt"

	"github.com/pocketbase/pocketbase/core"
	"github.com/samber/lo"
)

const GameCollection = "game"

func CreateGameRecord(app core.App, playerRecords []*core.Record) (*core.Record, error) {
	if len(playerRecords) < 2 {
		return nil, fmt.Errorf("cannot create game with less than 2 players (%d)", len(playerRecords))
	}

	collection, err := app.FindCollectionByNameOrId(GameCollection)
	if err != nil {
		return nil, err
	}

	game := core.NewRecord(collection)
	game.Set("players", lo.Map(playerRecords, func(p *core.Record, idx int) string {
		return p.GetString("accountName")
	}))

	return game, nil
}
