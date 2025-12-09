# Tag System
Very work in progress description

## Features
* TagManager creates optimized way of handling relationships between tags.
* TagContainer with add/remove/queries and also watch/unwatch for specific tags to respond to.
* TagRegistry stores string representation of tags for editor usage. Auto generates const for Tags so in GDScript you can access your tag Ability.Fireball with Tag.ABILITY_FIREBALL ( sometimes editor takes time to recognize so a reload of project is needed )


Build project with following command:
scons