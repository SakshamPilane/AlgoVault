#pragma once

class KeyValueStore;
class Persistence;

void startServer(KeyValueStore &store, Persistence &wal);