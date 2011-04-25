# -*- utf-8 -*-
#!/usr/bin/env ruby -Ku


require 'open-uri'
require 'csv'

if $0 == __FILE__ then
  # read all tsv
  Dir.glob("*.tsv").each { |tsv|
    CSV.open(tsv, 'r') { |row|
      p row
    }
  }

end
