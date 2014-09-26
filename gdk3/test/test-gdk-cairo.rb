# -*- coding: utf-8 -*-
#
# Copyright (C) 2014  Ruby-GNOME2 Project Team
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require "stringio"

class TestGdkCairo < Test::Unit::TestCase
  def setup
    @output = StringIO.new
    @surface = Cairo::PDFSurface.new(@output, 10, 10)
  end

  def test_set_source_color
    context = Cairo::Context.new(@surface)
    color = Gdk::Color.new(0xffff, 0x0000, 0xffff)
    context.source_color = color
    assert_equal(Cairo::Color::RGB.new(1.0, 0.0, 1.0),
                 context.source.color)
  end

  def test_set_source_rgba
    context = Cairo::Context.new(@surface)
    rgba = Gdk::RGBA.new(0.1, 0.2, 0.3, 0.4)
    context.source_rgba = rgba
    assert_equal(Cairo::Color::RGB.new(0.1, 0.2, 0.3, 0.4),
                 context.source.color)
  end
end