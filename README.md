Lando Acquia Recipe with Multisite Support
==========================================

This recipe with the included files can be used to create a multisite lando installation that supports the `lando pull` command.

Below is a list of all the files included in this repo, files within the tree below are marked as how they need to be added to your project.

|   | How to add to project            |
| - | -------------------------------- |
| ᵃ | Add to project                   |
| ᵉ | Edit file                        |
| ᵐ | Merge changes into existing file |
| ⁱ | Ignore file                      |

```
├── .landoᵃ
│   ├── acquia-pull.shᵃ
│   ├── database-generation.shᵃ
│   └── multisite.envᵃᵉ
├── .lando.ymlᵃᵐ
├── README.mdⁱ
├── docroot
│   └── sites
│       ├── site1
│       │   ├── settings.lando.phpᵃᵉ
│       │   └── settings.phpᵐ
│       ├── site2
│       │   ├── settings.lando.phpᵃᵉ
│       │   └── settings.phpᵐ
│       ├── sites.lando.phpᵃᵉ
│       └── sites.phpᵐ
└── drush
    └── sites
        └── self.site.ymlᵃᵉ
```

