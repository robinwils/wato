package main

import (
	"github.com/pocketbase/dbx"
	"github.com/pocketbase/pocketbase/core"
)

const MatchmakingCollection = "matchmaking_queue"

func Matchmaking(app core.App, teamSize, teamCount int) error {
	records, err := app.FindRecordsByFilter(MatchmakingCollection,
		"status = 'waiting' && teamSize = {:teamSize} && teamCount = {:teamCount}",
		"-created",
		teamCount*teamSize,
		0,
		dbx.Params{"teamSize": teamSize, "teamCount": teamCount},
	)
	if err != nil {
		return err
	}

	// got enough players in queue for the given type of game, create game
	if len(records) == teamCount*teamSize {
		err = app.RunInTransaction(func(txApp core.App) error {
			game, err := CreateGameRecord(app, records)
			if err != nil {
				return err
			}

			if err := txApp.Save(game); err != nil {
				return err
			}

			for _, record := range records {
				record.Set("status", "matched")
				record.Set("game", game.Id)
				if err := txApp.Save(record); err != nil {
					return err
				}
			}

			return nil
		})
	}

	return err
}
