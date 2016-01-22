# -*- mode: ruby -*-
# vi: set ft=ruby :

require "nokogiri"

module LagopusHooks
  class PrivateNetwork
    def self.replace_network_node(document)
      network = document.at_css("network")
      network["ipv6"] = "no"

      bridge = network.at_css("bridge")
      bridge["stp"] = "off"
    end

    def self.hook(str)
      document = Nokogiri::XML(str)
      PrivateNetwork.replace_network_node(document)

      return document.to_s
    end
  end

  def hook_private_network(str)
    return PrivateNetwork.hook(str)
  end
end
