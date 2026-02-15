package migrations

import (
	"github.com/pocketbase/pocketbase/core"
	m "github.com/pocketbase/pocketbase/migrations"
)

func init() {
	m.Register(func(app core.App) error {
		collection, err := app.FindCollectionByNameOrId("_pb_users_auth_")
		if err != nil {
			return err
		}

		// update field
		if err := collection.Fields.AddMarshaledJSONAt(0, []byte(`{
			"autogeneratePattern": "[a-f0-9]{8}",
			"hidden": false,
			"id": "text3208210256",
			"max": 8,
			"min": 8,
			"name": "id",
			"pattern": "^[a-f0-9]+$",
			"presentable": false,
			"primaryKey": true,
			"required": true,
			"system": true,
			"type": "text"
		}`)); err != nil {
			return err
		}

		return app.Save(collection)
	}, func(app core.App) error {
		collection, err := app.FindCollectionByNameOrId("_pb_users_auth_")
		if err != nil {
			return err
		}

		// update field
		if err := collection.Fields.AddMarshaledJSONAt(0, []byte(`{
			"autogeneratePattern": "[a-f0-9]{16}",
			"hidden": false,
			"id": "text3208210256",
			"max": 16,
			"min": 16,
			"name": "id",
			"pattern": "^[a-f0-9]+$",
			"presentable": false,
			"primaryKey": true,
			"required": true,
			"system": true,
			"type": "text"
		}`)); err != nil {
			return err
		}

		return app.Save(collection)
	})
}
