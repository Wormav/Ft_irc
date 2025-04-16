# Ft_irc

Une implémentation d'un serveur IRC (Internet Relay Chat) en C++.

## Architecture du Projet

Le projet est structuré en plusieurs composants, chacun responsable d'un aspect spécifique du serveur IRC :

- **Serveur** : Gère les connexions des clients, le traitement des commandes et le cycle de vie global du serveur.
- **Utilisateur** : Représente un client connecté, y compris son état d'authentification et son identité.
- **Canal** : Représente un canal IRC où les utilisateurs peuvent rejoindre et communiquer.
- **Commande** : Gère l'analyse et l'exécution des commandes du protocole IRC.
- **Bot** (Bonus) : Implémente un bot qui interagit avec les utilisateurs sur le serveur IRC.

### Relations entre les Composants

```
Serveur
 ├── gère plusieurs Utilisateurs (clients connectés)
 ├── maintient plusieurs Canaux
 └── utilise le gestionnaire de Commandes pour traiter les messages des clients
```

## Fonctionnalités du Code

### Flux de Travail du Serveur

1. **Initialisation** : Le serveur est initialisé avec un port et un mot de passe.
2. **Configuration du Socket** : Un socket est créé et configuré pour écouter les connexions entrantes.
3. **Boucle Principale** : Le serveur utilise `epoll` pour gérer plusieurs connexions de manière asynchrone.
4. **Gestion des Clients** :
   - Accepte les nouvelles connexions des clients.
   - Lit les données des clients et traite les commandes.
   - Envoie des réponses aux clients.
5. **Arrêt** : Libère les ressources lorsque le serveur est arrêté.

### Flux de Traitement des Commandes

1. Un client envoie une commande au serveur.
2. Le serveur lit la commande et la transmet au gestionnaire de `Commandes`.
3. Le gestionnaire de `Commandes` analyse la commande et exécute la méthode correspondante.
4. Les réponses sont renvoyées au client ou diffusées à d'autres clients si nécessaire.

## Documentation des Classes

### Classe Serveur

Gère les fonctionnalités principales du serveur IRC.

| Méthode | Description |
|---------|-------------|
| `Server(int port, const std::string& password)` | Initialise le serveur avec un port et un mot de passe. |
| `~Server()` | Libère les ressources. |
| `run()` | Démarre la boucle principale du serveur. |
| `disconnectClient(int client_fd)` | Déconnecte un client et libère ses ressources. |
| `cleanupResources()` | Libère toutes les ressources utilisées par le serveur. |
| `setupSocket()` | Configure le socket du serveur pour les connexions entrantes. |
| `handleNewConnection()` | Accepte une nouvelle connexion client. |
| `handleClientData(int client_fd)` | Traite les données reçues d'un client. |
| `processCommand(int client_fd, const std::string& line)` | Transmet une commande au gestionnaire de `Commandes`. |

---

### Classe Utilisateur

Représente un client connecté.

| Méthode | Description |
|---------|-------------|
| `User()` | Initialise un nouvel utilisateur. |
| `~User()` | Destructeur. |
| `getNickname() const` | Retourne le pseudonyme de l'utilisateur. |
| `getUsername() const` | Retourne le nom d'utilisateur de l'utilisateur. |
| `getRealname() const` | Retourne le vrai nom de l'utilisateur. |
| `isAuthenticated() const` | Vérifie si l'utilisateur est authentifié. |
| `isPasswordVerified() const` | Vérifie si l'utilisateur a vérifié le mot de passe du serveur. |
| `setNickname(const std::string& nick)` | Définit le pseudonyme de l'utilisateur. |
| `setUsername(const std::string& user)` | Définit le nom d'utilisateur de l'utilisateur. |
| `setRealname(const std::string& real)` | Définit le vrai nom de l'utilisateur. |
| `setAuthenticated(bool auth)` | Met à jour l'état d'authentification de l'utilisateur. |
| `setPasswordVerified(bool verified)` | Met à jour l'état de vérification du mot de passe. |
| `getFullIdentity() const` | Retourne la chaîne d'identité complète de l'utilisateur. |

---

### Classe Canal

Représente un canal IRC.

| Méthode | Description |
|---------|-------------|
| `Channel()` | Constructeur par défaut. |
| `Channel(const std::string& channelName)` | Initialise un canal avec un nom. |
| `~Channel()` | Destructeur. |
| `getName() const` | Retourne le nom du canal. |
| `getTopic() const` | Retourne le sujet du canal. |
| `getMembers() const` | Retourne l'ensemble des membres du canal. |
| `getOperators() const` | Retourne l'ensemble des opérateurs du canal. |
| `setName(const std::string& channelName)` | Définit le nom du canal. |
| `setTopic(const std::string& channelTopic)` | Définit le sujet du canal. |
| `addMember(int client_fd)` | Ajoute un membre au canal. |
| `removeMember(int client_fd)` | Supprime un membre du canal. |
| `hasMember(int client_fd) const` | Vérifie si un utilisateur est membre du canal. |
| `isEmpty() const` | Vérifie si le canal n'a pas de membres. |
| `addOperator(int client_fd)` | Ajoute un opérateur au canal. |
| `removeOperator(int client_fd)` | Supprime un opérateur du canal. |
| `isOperator(int client_fd) const` | Vérifie si un utilisateur est opérateur. |
| `addInvite(int client_fd)` | Invite un utilisateur au canal. |
| `removeInvite(int client_fd)` | Supprime une invitation pour un utilisateur. |
| `isInvited(int client_fd) const` | Vérifie si un utilisateur est invité. |
| `isInviteOnly() const` | Vérifie si le canal est en mode invitation uniquement. |
| `isTopicRestricted() const` | Vérifie si le sujet est restreint aux opérateurs. |
| `hasKeySet() const` | Vérifie si le canal a un mot de passe. |
| `hasUserLimitSet() const` | Vérifie si le canal a une limite d'utilisateurs. |
| `getKey() const` | Retourne le mot de passe du canal. |
| `getUserLimit() const` | Retourne la limite d'utilisateurs. |
| `setInviteOnly(bool value)` | Définit le mode invitation uniquement. |
| `setTopicRestricted(bool value)` | Définit le mode de restriction du sujet. |
| `setKey(const std::string& newKey)` | Définit le mot de passe du canal. |
| `removeKey()` | Supprime le mot de passe du canal. |
| `setUserLimit(size_t limit)` | Définit la limite d'utilisateurs. |
| `removeUserLimit()` | Supprime la limite d'utilisateurs. |
| `getModeString() const` | Retourne la chaîne des modes du canal. |
| `broadcastMessage(const std::string& message, int excludeClient)` | Envoie un message à tous les membres, en excluant un si spécifié. |

---

### Classe Commande

Gère l'analyse et l'exécution des commandes IRC.

| Méthode | Description |
|---------|-------------|
| `Command(Server* server, std::map<int, User>& users, std::map<std::string, Channel>& channels, const std::string& password)` | Initialise le gestionnaire de commandes. |
| `~Command()` | Destructeur. |
| `process(int client_fd, const std::string& line)` | Traite une commande d'un client. |
| `sendWelcomeMessages(int client_fd, const User& user)` | Envoie des messages de bienvenue à un utilisateur nouvellement authentifié. |
| `handlePass(int client_fd, std::istringstream& iss)` | Gère la commande `PASS`. |
| `handleNick(int client_fd, std::istringstream& iss)` | Gère la commande `NICK`. |
| `handleUser(int client_fd, const std::string& line)` | Gère la commande `USER`. |
| `handleJoin(int client_fd, std::istringstream& iss)` | Gère la commande `JOIN`. |
| `handlePrivmsg(int client_fd, std::istringstream& iss)` | Gère la commande `PRIVMSG`. |
| `handlePart(int client_fd, const std::string& line)` | Gère la commande `PART`. |
| `handleQuit(int client_fd, const std::string& line)` | Gère la commande `QUIT`. |
| `handleKick(int client_fd, const std::string& line)` | Gère la commande `KICK`. |
| `handleInvite(int client_fd, const std::string& line)` | Gère la commande `INVITE`. |
| `handleTopic(int client_fd, const std::string& line)` | Gère la commande `TOPIC`. |
| `handleMode(int client_fd, const std::string& line)` | Gère la commande `MODE`. |

---

### Classe Bot (Bonus)

Implémente un bot pour des interactions automatisées.

| Méthode | Description |
|---------|-------------|
| `Bot(const std::string& nickname, const std::string& username, const std::string& realname, const std::string& channel)` | Initialise le bot avec son identité et son canal. |
| `~Bot()` | Destructeur. |
| `initResponses()` | Initialise les réponses prédéfinies du bot. |
| `getRandomResponse() const` | Retourne une réponse aléatoire du bot. |
| `getNickname() const` | Retourne le pseudonyme du bot. |
| `getUsername() const` | Retourne le nom d'utilisateur du bot. |
| `getRealname() const` | Retourne le vrai nom du bot. |
| `getChannel() const` | Retourne le canal du bot. |

## Contributeurs

- [EricBrvs](https://github.com/EricBrvs)
- [aauberti](https://github.com/aauberti)
- [Mastau](https://github.com/Mastau)
- [Wormav](https://github.com/Wormav)
