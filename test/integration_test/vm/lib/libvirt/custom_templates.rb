# -*- mode: ruby -*-
# vi: set ft=ruby :

##
# overwrite: VagrantPlugins::ProviderLibvir::Util::ErbTemplate::to_xml().
# vagrant-libvirt-0.0.25(-0.0.30) is unsupported customizing detailed CPU, ..etc.
##
begin
  require "vagrant-libvirt/util/erb_template"

  # read hooks
  $hooks = {}
  Dir[File.join(File.dirname(__FILE__), "hooks", "*.rb")].each do |file|
    require_relative file
    name = File.basename(file, ".rb")
    $hooks[name] = "hook_#{name}"
  end

  module VagrantPlugins
    module ProviderLibvirt
      module Util
        module ErbTemplate
          include LagopusHooks

          def to_xml_new template_name = nil, data = binding
            erb = template_name || self.class.to_s.split("::").last.downcase
            str = to_xml_ori(template_name, data)

            if $hooks.has_key?(erb)
              puts "overwrite templates! (#{erb})"
              str = send($hooks[erb], str)
            end
            return str
          end

          alias_method :to_xml_ori, :to_xml
          alias_method :to_xml, :to_xml_new
        end
      end
    end
  end
rescue LoadError
end
