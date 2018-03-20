# lando-acquia-multisite-recipe
A recipe for Lando to replicate an Acquia site

To make sql synchronization between upstream and your lando installation you need to add your credentials from the Acquia servers.

 1. Download your Drush credentials and aliases via Acquia cloud.
     1. [Login](https://accounts.acquia.com/user) to Acquia Cloud
     1. Click on your name in the upper right menu
     1. Click on the *profile* link
     1. Click on the *Credentials* tab
     1. Under the heading *Drush Integration* save the tarball linked under the *Download Drush aliases* link and save the tarball into the git project root. It should be named `acquiacloud.tar.gz`.
