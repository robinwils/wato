package main

import (
	"log"
	"os"

	"github.com/pocketbase/dbx"
	"github.com/pocketbase/pocketbase"
	"github.com/pocketbase/pocketbase/apis"
	"github.com/pocketbase/pocketbase/core"
	"github.com/pocketbase/pocketbase/plugins/migratecmd"

	_ "watohttpd/migrations"
)

func main() {
	app := pocketbase.New()

	autoMigrate := os.Getenv("AUTOMIGRATE")

	migratecmd.MustRegister(app, app.RootCmd, migratecmd.Config{
		// enable auto creation of migration files when making collection changes in the Dashboard
		// (the isGoRun check is to enable it only during development)
		Automigrate: autoMigrate != "1",
	})

	app.OnServe().BindFunc(func(se *core.ServeEvent) error {
		// serves static files from the provided public dir (if exists)
		se.Router.GET("/{path...}", apis.Static(os.DirFS("./pb_public"), false))

		return se.Next()
	})

	app.OnRecordCreateRequest("matchmaking_queue").BindFunc(func(e *core.RecordRequestEvent) error {
		playerId := e.Record.GetString("accountName")
		existing, _ := e.App.FindFirstRecordByFilter(MatchmakingCollection,
			"accountName = {:player}",
			dbx.Params{"player": playerId},
		)
		if existing != nil {
			if err := e.App.Delete(existing); err != nil {
				return err
			}
		}
		return e.Next()
	})

	app.OnRecordAfterCreateSuccess("matchmaking_queue").BindFunc(func(e *core.RecordEvent) error {
		err := Matchmaking(e.App, e.Record.GetInt("teamSize"), e.Record.GetInt("teamCount"))
		if err != nil {
			return err
		}

		return e.Next()
	})

	if err := app.Start(); err != nil {
		log.Fatal(err)
	}
}
