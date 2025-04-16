#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel
{
	private:
		std::string name;
		std::string topic;
		std::set<int> members;
		std::set<int> operators;
		std::set<int> invited;

		bool inviteOnly;
		bool topicRestricted;
		bool hasUserLimit;
		bool hasKey;
		std::string key;
		size_t userLimit;

	public:
		Channel();
		explicit Channel(const std::string& channelName);
		~Channel();

		const std::string& getName() const;
		const std::string& getTopic() const;
		const std::set<int>& getMembers() const;
		const std::set<int>& getOperators() const;

		void setName(const std::string& channelName);
		void setTopic(const std::string& channelTopic);

		bool addMember(int client_fd);
		bool removeMember(int client_fd);
		bool hasMember(int client_fd) const;
		bool isEmpty() const;

		bool addOperator(int client_fd);
		bool removeOperator(int client_fd);
		bool isOperator(int client_fd) const;

		bool addInvite(int client_fd);
		bool removeInvite(int client_fd);
		bool isInvited(int client_fd) const;

		bool isInviteOnly() const;
		bool isTopicRestricted() const;
		bool hasKeySet() const;
		bool hasUserLimitSet() const;
		const std::string& getKey() const;
		size_t getUserLimit() const;

		void setInviteOnly(bool value);
		void setTopicRestricted(bool value);
		void setKey(const std::string& newKey);
		void removeKey();
		void setUserLimit(size_t limit);
		void removeUserLimit();

		std::string getModeString() const;

		void broadcastMessage(const std::string& message,
			int excludeClient = -1) const;
};

#endif
