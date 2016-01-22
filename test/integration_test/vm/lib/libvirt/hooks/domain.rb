# -*- mode: ruby -*-
# vi: set ft=ruby :

require "nokogiri"

module LagopusHooks
  class Domain
    def self.add_child_node(pnode, document, name, attrs, content)
      node = Nokogiri::XML::Node.new(name, document)

      if attrs
        attrs.each_pair do |k, v|
          node[k] = v
        end
      end

      if content
        node.content = content
      end

      pnode << node
    end

    def self.replace_cpu_node(document)
      vcpu = document.at_css("vcpu")
      cpu = document.at_css("cpu")

      # delete child nodes in "cpu".
      cpu.children.each{|node|
        node.remove
      }

      # [[node_name, attrs, content], ..]
      nodes = [["model", {"fallback" => "allow"}, "SandyBridge"],
               ["vendor", nil, "Intel"],
               ["topology", {"sockets" => vcpu.content, "cores" => vcpu.content,
                  "threads" => "1"}, nil],
               ["feature", {"policy" => "require", "name" => "vme"}, nil],
               ["feature", {"policy" => "require", "name" => "dtes64"}, nil],
               ["feature", {"policy" => "require", "name" => "vmx"}, nil],
               ["feature", {"policy" => "require", "name" => "erms"}, nil],
               ["feature", {"policy" => "require", "name" => "xtpr"}, nil],
               ["feature", {"policy" => "require", "name" => "smep"}, nil],
               ["feature", {"policy" => "require", "name" => "pcid"}, nil],
               ["feature", {"policy" => "require", "name" => "est"}, nil],
               ["feature", {"policy" => "require", "name" => "monitor"}, nil],
               ["feature", {"policy" => "require", "name" => "smx"}, nil],
               ["feature", {"policy" => "require", "name" => "tm"}, nil],
               ["feature", {"policy" => "require", "name" => "acpi"}, nil],
               ["feature", {"policy" => "require", "name" => "osxsave"}, nil],
               ["feature", {"policy" => "require", "name" => "ht"}, nil],
               ["feature", {"policy" => "require", "name" => "pdcm"}, nil],
               ["feature", {"policy" => "require", "name" => "fsgsbase"}, nil],
               ["feature", {"policy" => "require", "name" => "f16c"}, nil],
               ["feature", {"policy" => "require", "name" => "ds"}, nil],
               ["feature", {"policy" => "require", "name" => "tm2"}, nil],
               ["feature", {"policy" => "require", "name" => "ss"}, nil],
               ["feature", {"policy" => "require", "name" => "pbe"}, nil],
               ["feature", {"policy" => "require", "name" => "ds_cpl"}, nil],
              ]

      # add child nodes.
      nodes.each do |node|
        add_child_node(cpu, document, node[0], node[1], node[2])
      end
    end

    def self.hook(str)
      document = Nokogiri::XML(str)
      replace_cpu_node(document)

      return document.to_s
    end
  end

  def hook_domain(str)
    return Domain.hook(str)
  end
end
